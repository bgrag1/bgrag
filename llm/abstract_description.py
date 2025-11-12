import torch
import os
import json
from tqdm import tqdm
from transformers import AutoModelForCausalLM, AutoTokenizer

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
    

def generate_prompt(description):
    promt = 'With the detailed vulnerability knowledge extracted from the cve description or commit message, your task is to abstract and generalize this knowledge to enhance its applicability across different scenarios. Please adhere to the following guidelines and examples provided:\n'
    
    guidelines = """ 
    GUIDELINES:
    • Abstracting Method Invocations. The extracted knowledge might contain concrete method invocations with detailed function identifiers (e.g., io_worker_handle_work function) and parameters (e.g., mutex_lock(&dmxdev->mutex)), which can be abstracted into the generalized description (e.g., "during handling of IO work processes" and "employing a locking mechanism akin to mutex_lock()").
    • Abstracting Variable Names and Types. The extracted knowledge might contain concrete variable names or types (e.g., "without &dev->ref initialization"), which can be abstracted into the more general description (e.g., "without proper reference counter initialization").
    • Abstracting File Paths. The extracted knowledge might contain concrete file paths (e.g., "drivers/gpu/drm/amd/amdgpu/amdgpu_cs.c"), which can be abstracted into the more general description (e.g., "within the AMD GPU driver").
    • Abstracting Version Numbers. The extracted knowledge might contain concrete version numbers (e.g., "In the Linux kernel before 6.4.12"), which can be abstracted into the more general description (e.g., "in the Linux kernel prior to a specific version").
    """

    examples = """
    EXAMPLE1:
    input: "Nginx NJS v0.7.2 was discovered to contain a segmentation violation in the function njs_array_convert_to_slow_array at src/njs_array.c.."
    output: "It was discovered to contain a segmentation violation occurring within a specific function responsible for array conversion."
    
    EXAMPLE2:
    input: "The selinux_ip_postroute_iptables_compat function in security/selinux/hooks.c in the SELinux subsystem in the Linux kernel before 2.6.27.22, and 2.6.28.x before 2.6.28.10, when compat_net is enabled, omits calls to avc_has_perm for the (1) node and (2) port, which allows local users to bypass intended restrictions on network traffic. NOTE: this was incorrectly reported as an issue fixed in 2.6.27.21."
    output: "A function handling IP post-routing within the SELinux subsystem of the Linux kernel, when a network compatibility feature is enabled, fails to perform necessary permission checks for network endpoints like nodes and ports. This oversight allows local users to bypass intended network traffic restrictions."
    """

    inf = f"""
    Now, please provide the abstracted and generalized description of the following vulnerability knowledge:
    <Desription begin>
    {description}
    <Description end>
"""

    return promt + guidelines + examples + inf

def clear_abstraction():
    path = '/home/m2024-djj/Vulrag/rag/abstract_description'
    for cwe in os.listdir(path):
        for repo in os.listdir(os.path.join(path, cwe)):
            repo_path = os.path.join(path, cwe, repo)
            for file in os.listdir(repo_path):
                file_path = os.path.join(repo_path, file)
                save_path = file_path[:-5] + '_clear.json'
                if not file.endswith('.json'):
                    continue
                with open(file_path, 'r') as f:
                    data = json.load(f)
                commit = data['commit']
                description = data['description']
                data_clear = {'commit': commit.replace('output:', '').replace('\"', '').replace("`", "'"), 'description': description.replace('output:', '').replace('\"', '').replace("`", "'")}
                with open(save_path, 'w') as f:
                    json.dump(data_clear, f, indent=4)

def main():
    print('---------------Task start-----------------')
    # 0. 加载模型
    model = ChatModel()
    # 1. 读取数据
    data_path = '/home/m2024-djj/Vulrag/rag/cwe_dir'
    save_path = '/home/m2024-djj/Vulrag/rag/abstract_description'
    for cwe in tqdm(os.listdir(data_path), desc='CWE'):
        if not cwe.startswith('CWE-'):
            continue
        with open(os.path.join(data_path, cwe), 'r') as f:
            data = json.load(f)
        save_path_cwe = os.path.join(save_path, cwe.replace('.json', ''))
        if not os.path.exists(save_path_cwe):
            os.makedirs(save_path_cwe)
        # 2. 遍历数据
        for vul_inf in tqdm(data,desc=f'now {cwe}'):
            commit_inf = (vul_inf['commit_title'] + '.' + vul_inf['commit_text']).replace('..', '.')
            description = vul_inf['description']
            repo, name = vul_inf['func_name'].split('/')[0], vul_inf['func_name'].split('/')[-1]
            save_path_repo = os.path.join(save_path_cwe, repo)
            if not os.path.exists(save_path_repo):
                os.makedirs(save_path_repo) 
            save_path_file = os.path.join(save_path_repo, name+'.json')
            if os.path.exists(save_path_file):
                continue
            # 3. 生成提示
            prompt_commit = generate_prompt(commit_inf)
            prompt_description = generate_prompt(description)
            # 4. 生成结果
            response_commit = model.generate(prompt_commit)
            response_description = model.generate(prompt_description)
            # 5. 保存结果
            with open(save_path_file, 'w') as f:
                json.dump({"commit": response_commit, "description": response_description}, f, indent=4)
        

if __name__ == "__main__":
    # main()
    clear_abstraction()