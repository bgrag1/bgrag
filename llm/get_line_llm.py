import os
from tqdm import tqdm
import torch
import ijson
import json
import logging
from transformers import AutoModelForCausalLM, AutoTokenizer
from linenum_to_code import *
import re

# 使用qwen模型提取diff行中和漏洞相关的行

logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - %(name)s - %(levelname)s - %(message)s',
    handlers=[
        logging.FileHandler('vulrag.log'),
        logging.StreamHandler()
    ]
)
logger = logging.getLogger(__name__)

home_path = "/home/m2024-djj/llm"
model_name = "/home/m2024-djj/llm/model/Qwen2.5-Coder-14B-Instruct"

class ChatModel:
    def __init__(self, model=model_name):
        self.device = "cuda:1"
        self.model = AutoModelForCausalLM.from_pretrained(
            model,
            torch_dtype=torch.float16
        ).to(self.device)
        self.tokenizer = AutoTokenizer.from_pretrained(model)

    def generate(self, prompt):
        messages = [
                {"role": "system", "content": "You are Qwen, created by Alibaba Cloud. You are a helpful assistant."},
                {"role": "user", "content": prompt}]
    
        text = self.tokenizer.apply_chat_template(
                messages,
                tokenize=False,
                add_generation_prompt=True,
            )
        
        model_inputs = self.tokenizer([text], return_tensors="pt").to(self.device)

        generated_ids = self.model.generate( 
                    **model_inputs,
                    max_new_tokens=2048,
                    do_sample = False,
                    temperature = None,
                    top_p = None,
                    top_k = None,
            )

        generated_ids = [
            output_ids[len(input_ids):] for input_ids, output_ids in zip(model_inputs.input_ids, generated_ids)
        ]

        response = self.tokenizer.batch_decode(generated_ids, skip_special_tokens=True)[0]
        return response

def process_llm_response(cwe_id, repo_name, func_name, response):
    lines = response.split('\n')
    added_pattern = r"^\d+\.\s?\(added line\)\s*(.*)$"
    deleted_pattern = r"^\d+\.\s?\(deleted line\)\s*(.*)$"
    added_lines = []
    deleted_lines = []
    unmatched_lines = []

    for line in lines:
        if len(line) < 14:
            continue
        match1 = re.match(added_pattern, line.strip())
        match2 = re.match(deleted_pattern, line.strip())
        if match1:
            # 有的输出的开头和结尾有'
            text = match1.group(1).strip().strip("'")
            if len(text) > 1:
                added_lines.append(text)
        elif match2:
            text = match2.group(1).strip().strip("'")
            if len(text) > 1:
                deleted_lines.append(text)
        else:
            unmatched_lines.append(line.strip())
    if unmatched_lines:
        logger.warning(f"{cwe_id}:{repo_name}/{func_name}: Unmatched lines: {unmatched_lines}")
    return added_lines, deleted_lines

def process_diff_line(lines):
    for i in range(len(lines)):
        lines[i] = lines[i].strip().strip('\t')
    return lines

def generate_prompt(func_name, description, commit_msg, added_line, deleted_line):
    prompt0 = """
    You are an expert in vulnerability analysis. Please combine your existing security knowledge with the following tips for vulnerability identification to determine which lines in the code changes are most likely related to vulnerabilities or vulnerability fixes. Output in the form of a numbered list, each item strictly formatted as:"X.(added/deleted line) [code line]".
    """

    tips = """
    TIPS:
    - Only lines which are most likely directly or indirectly related to vulnerabilities or vulnerability fixes should be output.
    - Only output lines from the given 'Added line' and 'Deleted line'.
    - In the added lines and the deleted lines, code lines that involve API calls, other function calls, or memory operations(pointer、array...) are more likely to be related to vulnerabilities.
    - The commit message and CVE description provide additional context to help you identify the vulnerability-related lines.
    - Do not output additional characters in the output line, such as '`', '\'. Only output the original characters in the line.
    """

    example = """
    Here is an example to help you understand your task better:
    example's function name: "rndis_query_responset"
    example's cve description: "Multiple integer overflows in the USB Net device emulator (hw/usb/dev-network.c) in QEMU before 2.5.1 allow local guest OS administrators to cause a denial of service (QEMU process crash) or obtain sensitive host memory information via a remote NDIS control message packet that is mishandled in the (1) rndis_query_response, (2) rndis_set_response, or (3) usb_net_handle_dataout function."
    example's commit message: "usb: check RNDIS buffer offsets & length.When processing remote NDIS control message packets,the USB Net device emulator uses a fixed length(4096) data buffer.The incoming informationBufferOffset & Length combination couldoverflow and cross that range. Check control message bufferoffsets and length to avoid it."
    example's deleted line: ["if (bufoffs + buflen > length)"] 
    example's added line: ["if (buflen > length || bufoffs >= length || bufoffs + buflen > length) {", "}"] 
    OUTPUT: 1.(deleted line) if (bufoffs + buflen > length) \n2.(added line) if (buflen > length || bufoffs >= length || bufoffs + buflen > length) {\n
    """
    

    inf = f"""
    Now, Complete the task through the following information.Please strictly follow the output format in the example.
    function name: "{func_name}"
    cve description: "{description}"
    commit message: "{commit_msg}"
    Added line: {added_line}
    deleted line: {deleted_line}
    """
    my_prompt = prompt0 + tips + example + inf
    return my_prompt

def generate_prompt0(func_before, func_after, description, commit_msg, added_line, deleted_line):
    prompt0 = """
    You are an expert in vulnerability analysis. Please combine your existing security knowledge with the following tips for vulnerability identification to determine which lines in the code changes are most likely related to vulnerabilities or vulnerability fixes. Output in the form of a numbered list, each item strictly formatted as:"X.(added/deleted line) [code line]".
    """

    # tips = """
    # TIPS:
    # - Only lines which are most likely directly or indirectly related to vulnerabilities or vulnerability fixes should be output.
    # - In the added lines and the deleted lines, code lines that involve API calls, other function calls, or memory operations(pointer、array...) are more likely to be related to vulnerabilities.
    # - The commit message and CVE description provide additional context to help you identify the vulnerability-related lines.
    # - Do not output the comment lines.
    # - Do not combine multiple lines into one line output.For example, there are two line breaks in "register Image \n*image,\n *next", so output them separately.
    # - Do not add or delete any characters in the output line, such as '{', '`', '(', '\'. Only output the original characters in the line.
    # """
    tips = """
    TIPS:
    - Only lines which are most likely directly or indirectly related to vulnerabilities or vulnerability fixes should be output.
    - Only output lines from the given 'Added line' and 'Deleted line'.
    - In the added lines and the deleted lines, code lines that involve API calls, other function calls, or memory operations(pointer、array...) are more likely to be related to vulnerabilities.
    - The commit message and CVE description provide additional context to help you identify the vulnerability-related lines.
    - Do not output the comment lines.
    """


    example = """
    Here is an example to help you understand your task better:
    Example's cve description: "Multiple integer overflows in the USB Net device emulator (hw/usb/dev-network.c) in QEMU before 2.5.1 allow local guest OS administrators to cause a denial of service (QEMU process crash) or obtain sensitive host memory information via a remote NDIS control message packet that is mishandled in the (1) rndis_query_response, (2) rndis_set_response, or (3) usb_net_handle_dataout function."
    Example's commit message: "usb: check RNDIS buffer offsets & length.When processing remote NDIS control message packets,the USB Net device emulator uses a fixed length(4096) data buffer.The incoming informationBufferOffset & Length combination couldoverflow and cross that range. Check control message bufferoffsets and length to avoid it."
   
    **Example's function before change**:

    <Function start>
    static int rndis_query_response(USBNetState *s,\n                rndis_query_msg_type *buf, unsigned int length)\n{\n    rndis_query_cmplt_type *resp;\n    /* oid_supported_list is the largest data reply */\n    uint8_t infobuf[sizeof(oid_supported_list)];\n    uint32_t bufoffs, buflen;\n    int infobuflen;\n    unsigned int resplen;\n\n    bufoffs = le32_to_cpu(buf->InformationBufferOffset) + 8;\n    buflen = le32_to_cpu(buf->InformationBufferLength);\n    if (bufoffs + buflen > length)\n        return USB_RET_STALL;\n\n    infobuflen = ndis_query(s, le32_to_cpu(buf->OID),\n                            bufoffs + (uint8_t *) buf, buflen, infobuf,\n                            sizeof(infobuf));\n    resplen = sizeof(rndis_query_cmplt_type) +\n            ((infobuflen < 0) ? 0 : infobuflen);\n    resp = rndis_queue_response(s, resplen);\n    if (!resp)\n        return USB_RET_STALL;\n\n    resp->MessageType = cpu_to_le32(RNDIS_QUERY_CMPLT);\n    resp->RequestID = buf->RequestID; /* Still LE in msg buffer */\n    resp->MessageLength = cpu_to_le32(resplen);\n\n    if (infobuflen < 0) {\n        /* OID not supported */\n        resp->Status = cpu_to_le32(RNDIS_STATUS_NOT_SUPPORTED);\n        resp->InformationBufferLength = cpu_to_le32(0);\n        resp->InformationBufferOffset = cpu_to_le32(0);\n        return 0;\n    }\n\n    resp->Status = cpu_to_le32(RNDIS_STATUS_SUCCESS);\n    resp->InformationBufferOffset =\n            cpu_to_le32(infobuflen ? sizeof(rndis_query_cmplt_type) - 8 : 0);\n    resp->InformationBufferLength = cpu_to_le32(infobuflen);\n    memcpy(resp + 1, infobuf, infobuflen);\n\n    return 0;\n}
    <Function end>

    **Example's function after change**:
    <Function start>
    "static int rndis_query_response(USBNetState *s,\n                rndis_query_msg_type *buf, unsigned int length)\n{\n    rndis_query_cmplt_type *resp;\n    /* oid_supported_list is the largest data reply */\n    uint8_t infobuf[sizeof(oid_supported_list)];\n    uint32_t bufoffs, buflen;\n    int infobuflen;\n    unsigned int resplen;\n\n    bufoffs = le32_to_cpu(buf->InformationBufferOffset) + 8;\n    buflen = le32_to_cpu(buf->InformationBufferLength);\n    if (buflen > length || bufoffs >= length || bufoffs + buflen > length) {\n        return USB_RET_STALL;\n    }\n\n    infobuflen = ndis_query(s, le32_to_cpu(buf->OID),\n                            bufoffs + (uint8_t *) buf, buflen, infobuf,\n                            sizeof(infobuf));\n    resplen = sizeof(rndis_query_cmplt_type) +\n            ((infobuflen < 0) ? 0 : infobuflen);\n    resp = rndis_queue_response(s, resplen);\n    if (!resp)\n        return USB_RET_STALL;\n\n    resp->MessageType = cpu_to_le32(RNDIS_QUERY_CMPLT);\n    resp->RequestID = buf->RequestID; /* Still LE in msg buffer */\n    resp->MessageLength = cpu_to_le32(resplen);\n\n    if (infobuflen < 0) {\n        /* OID not supported */\n        resp->Status = cpu_to_le32(RNDIS_STATUS_NOT_SUPPORTED);\n        resp->InformationBufferLength = cpu_to_le32(0);\n        resp->InformationBufferOffset = cpu_to_le32(0);\n        return 0;\n    }\n\n    resp->Status = cpu_to_le32(RNDIS_STATUS_SUCCESS);\n    resp->InformationBufferOffset =\n            cpu_to_le32(infobuflen ? sizeof(rndis_query_cmplt_type) - 8 : 0);\n    resp->InformationBufferLength = cpu_to_le32(infobuflen);\n    memcpy(resp + 1, infobuf, infobuflen);\n\n    return 0;\n}
    <Function end>

    example's deleted line: ["if (bufoffs + buflen > length)"] 
    example's added line: ["if (buflen > length || bufoffs >= length || bufoffs + buflen > length) {", "}"] 
    OUTPUT: 1.(deleted line) if (bufoffs + buflen > length) \n2.(added line) if (buflen > length || bufoffs >= length || bufoffs + buflen > length) {\n
    """
    

    inf = f"""
    Now, Complete the task through the following information.Please strictly follow the output format in the example.
    CVE description: "{description}"
    Commit message: "{commit_msg}"
    Function before change:

    <Function start>
    {func_before}
    <Function end>

    Function after change:

    <Function start>
    {func_after}
    <Function end> 

    Added line: {added_line}
    Deleted line: {deleted_line}
    """
    my_prompt = prompt0 + tips + example + inf
    return my_prompt

def main():
    print('---------------Task start-----------------')
    # 0. 加载模型
    model = ChatModel()
    # 1. 读取数据(commit_msg, addded_line, deleted_line)
    big_func_save_path = '/home/m2024-djj/Vulrag/rag/big_func.txt'
    vul_inf_path = '/home/m2024-djj/Vulrag/rag/cwe_dir'
    save_dir = '/home/m2024-djj/Vulrag/rag/diff_lines'
    json_files = []
    for file in os.listdir(vul_inf_path):
        if file.startswith('CWE') and file.endswith('.json'):
            json_files.append(file)

    for cwe in tqdm(json_files, desc=f'Now Processing:'):
        with open(os.path.join(vul_inf_path, cwe), 'r') as f:
            data = json.load(f)
        save_dir_cwe = os.path.join(save_dir, cwe[:-4])
        if not os.path.exists(save_dir_cwe):
            os.makedirs(save_dir_cwe)
         
        for item in tqdm(data, desc=f'Now Processing {cwe}:'):
            git_url = item['git_url']
            description = item['description']
            repo_name, func_name = item['func_name'].split('/')[0], item['func_name'].split('/')[-1]
            commit_msg = item['commit_title'] + ' ' + item['commit_text']
            added_line = process_diff_line(item["diff_line_info"]['added_lines'])
            deleted_line = process_diff_line(item["diff_line_info"]['deleted_lines'])
            func_before = item['func_before']
            func = item['func']
            repo_path = os.path.join(save_dir_cwe, repo_name)
            func_path = os.path.join(repo_path, f'{func_name}.json')

            # 跳过没问题的
            if os.path.exists(func_path):
                continue

            # 2. 生成prompt
            # prompt = generate_prompt0(func_before, func, description, commit_msg, added_line, deleted_line)
            # if too big
            # if len(prompt) > 32768:
            #     with open(big_func_save_path, 'a', encoding='utf-8') as file:
            #         information_big_func = cwe + ' ' + repo_name + ' ' + func_name + '\n'
            #         file.write(information_big_func)
            #     continue
            prompt = generate_prompt(func_name, description, commit_msg, added_line, deleted_line)
  
            # 3. 生成response
            response = model.generate(prompt)
            # 4. 处理response，得到added行和deleted行
            llm_added_lines, llm_deleted_lines = process_llm_response(cwe, repo_name, func_name, response)
            # 5. 根据added行和deleted行，找到llm行号、原行号
            llm_added_line_numbers, llm_deleted_line_numbers = line_to_number(cwe, repo_name,func_name, git_url, llm_added_lines, llm_deleted_lines, func_before, func)
            added_line_numbers, deleted_line_numbers = line_to_number(cwe, repo_name,func_name, git_url, added_line, deleted_line, func_before, func)
            # 6. 保存行号
            if not os.path.exists(repo_path):
                os.makedirs(repo_path)

            ret_temp = {
                'llm_added_lines': llm_added_line_numbers,
                'llm_deleted_lines': llm_deleted_line_numbers, 
                'actual_added_lines': added_line_numbers,
                'actual_deleted_lines': deleted_line_numbers
            }

            with open(func_path, 'w') as f:
                json.dump(ret_temp, f, indent=4)
        print(f'{cwe} Done!')


if  __name__ == '__main__':
    main()
