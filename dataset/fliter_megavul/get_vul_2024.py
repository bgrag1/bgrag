import os
import json
from tqdm import tqdm  

def get_2024_vul():
    dataset_path = '/home/m2024-djj/Vulrag/rag/cwe_dir'
    # 读取每个cwe
    save_path_vul = '/home/m2024-djj/Vulrag/rag/train/test/vul'
    save_path_novul = '/home/m2024-djj/Vulrag/rag/train/test/novul'
    if not os.path.exists(save_path_vul):
        os.makedirs(save_path_vul)
    if not os.path.exists(save_path_novul):
        os.makedirs(save_path_novul)
    len_vul_after_fliter = 0
    len_vul = 0
    len_novul_after_fliter = 0
    len_novul = 0
    for cwe in tqdm(os.listdir(dataset_path), desc='cwe'):
        if not cwe.startswith('CWE'):
            continue
        cwe_path = os.path.join(dataset_path, cwe)
        with open(cwe_path, 'r') as f:
            data = json.load(f)
        for inf in data:
            cve = inf['cve_id']
            year = cve.split('-')[1]
            if year != '2024':
                continue
            len_vul += 1
            len_novul += 1
            func_name = inf['func_name'].split('/')[-1]
            #保存CWE号
            with open('/home/m2024-djj/Vulrag/rag/test_data_secvul/fliter_megavul/record_2024.txt', 'a') as f:
                f.write(f"{func_name.replace('::','__') +'.c'} -- {inf['func_name'].split('/')[0]} -- {cwe}\n")

    #         # 处理vul
    #         with open(os.path.join(save_path_vul, '1_' + func_name.replace('::','__') +'.c'), 'a') as f:
    #             f.write(inf['func_before'])
    #         len_vul_after_fliter += 1
    #         # 处理novul
    #         with open(os.path.join(save_path_novul, '0_' + func_name.replace('::','__') +'.c'), 'a') as f:
    #             f.write(inf['func'])
    #         len_novul_after_fliter += 1
    # print(f"novul:{len_novul_after_fliter}/{len_novul}, vul: {len_vul_after_fliter}/{len_vul}")

def get_real_vul_line_2024():
    file_path = '/home/m2024-djj/Vulrag/rag/test_data_secvul/fliter_megavul/record_2024.txt'
    save_path = '/home/m2024-djj/Vulrag/rag/test_data_secvul/fliter_megavul/real_vul_line_2024.txt'
    with open(file_path, 'r') as f:
        lines = f.readlines()
    for line in lines:
        func_name = line.split(' -- ')[0]
        repo = line.split(' -- ')[1].strip()
        cwe = line.split(' -- ')[2].strip()
        cwe_path = os.path.join('/home/m2024-djj/Vulrag/rag/diff_lines', cwe.replace('json', ""), repo, func_name.replace('.c', '') + ".json")
        cwe_path2 = os.path.join('/home/m2024-djj/Vulrag/rag/diff_lines', cwe.replace('json', ""), repo, func_name.replace('__', '::').replace('.c', '') + ".json")
        try:
            with open(cwe_path, 'r') as f:
                data = json.load(f)
            deleted_lines = data.get('actual_deleted_lines', [])
            llm_vul_lines = data.get('llm_deleted_lines', [])

        except:
            try:
                with open(cwe_path2, 'r') as f:
                    data = json.load(f)
                deleted_lines = data.get('actual_deleted_lines', [])
                llm_vul_lines = data.get('llm_deleted_lines', [])
            except:
                print(f"not found {cwe_path} and {cwe_path2}")
                continue
        with open(save_path, 'a') as f:
            f.write(f"{func_name} -- {deleted_lines} -- {llm_vul_lines}\n")

# 获取vul行
def get_vul_line():
    real_record_path = '/home/m2024-djj/Vulrag/rag/test_data_secvul/fliter_megavul/real_vul_line_2024.txt'
    name = {}
    file_path = {}
    with open(real_record_path, 'r') as f:
        lines = f.readlines()
    for line in lines:
        func_name = line.split(' -- ')[0]
        deleted_lines = line.split(' -- ')[1].strip()
        llm_vul_lines = line.split(' -- ')[2].strip()
        try:
            deleted_lines_list = json.loads(deleted_lines.replace("'", '"'))
        except:
            deleted_lines_list = []
        try:
            llm_vul_lines_list = json.loads(llm_vul_lines.replace("'", '"'))
        except:
            llm_vul_lines_list = []

        name[func_name] = {
            "deleted_lines": deleted_lines_list,
            "llm_vul_lines": llm_vul_lines_list
        }
    return name

# 获取slice到的行
def get_slice_line():
    # 获取函数的CWE和repoh
    has_tran = False
    real_record_path = '/home/m2024-djj/Vulrag/rag/test_data_secvul/fliter_megavul/record_2024.txt'
    with open(real_record_path, 'r') as f:
        lines = f.readlines()
    record = {}
    for line in lines:
        func_name = line.split(' -- ')[0]
        repo = line.split(' -- ')[1].strip()
        cwe = line.split(' -- ')[2].strip()
        record[func_name] = {
            "repo": repo,
            "cwe": cwe
        }
    
    # slice_path = '/home/m2024-djj/Vulrag/rag/output_dict' + cwe + repo + func_name_before
    llm_ret_path = '/home/m2024-djj/Vulrag/rag/test_data_secvul/fliter_megavul/result/DeepSeek_vul_1_explanation_3.json'
    with open(llm_ret_path, 'r') as f:
        llm_data = json.load(f)
    slice_lines = {}
    for inf in llm_data:
        if inf[2:] in record:
            now_func_name = inf[2:]
        elif inf[2:].replace("::", "__") in record:
            now_func_name = inf[2:].replace("::", "__")
            init_func_name = inf[2:] 
            has_tran = True
        else:
            print(f"not found {inf.split('1_')[1]}")
            continue
        slice_lines[now_func_name] = []
        # 找到切片
        try:
            if has_tran:
                has_tran = False
                slice_path = os.path.join('/home/m2024-djj/Vulrag/rag/output_dict', 
                                        record[now_func_name]['cwe'].replace('.json', ""), 
                                        record[now_func_name]['repo'], 
                                        init_func_name.replace('.c', '') + "_before")
            else:
                slice_path = os.path.join('/home/m2024-djj/Vulrag/rag/output_dict', 
                                        record[now_func_name]['cwe'].replace('.json', ""), 
                                        record[now_func_name]['repo'], 
                                        now_func_name.replace('.c', '') + "_before")
            for file in os.listdir(slice_path):
                if file.endswith('.json'):
                    slice_file_path = os.path.join(slice_path, file)
                    # print(f"Reading slice file: {slice_file_path}")
                    with open(slice_file_path, 'r') as f:
                        slice_data = json.load(f)
                    slice_lines[now_func_name].extend(slice_data)
                    slice_lines[now_func_name] = list(set(slice_lines[now_func_name]))
            slice_lines[now_func_name].sort()
        except Exception as e:
            print(f"Error processing {now_func_name}: {e}")
            continue
        
    return slice_lines, record

def analyze_llm_explanation():    
    all_num = 0
    IoU = 0
    # 获取真实的vul行
    # name = get_vul_line()

    # 获取真实的slice行
    slice_lines, record = get_slice_line()
    
    llm_ret_path = '/home/m2024-djj/Vulrag/rag/test_data_secvul/fliter_megavul/result/DeepSeek_vul_1_explanation_2.json'
    with open(llm_ret_path, 'r') as f:
        llm_data = json.load(f)
    for inf in llm_data:
        if inf[2:] not in slice_lines and inf[2:].replace("::", "__") not in slice_lines:
            print(f"not found {inf.split('1_')[1]}")
            continue
        try:
            if slice_lines[inf[2:]] != []:
                all_num += 1
                print(f"{inf[2:]}有切片")
        except:
            if slice_lines[inf[2:].replace("::", "__")] != [] :
                all_num += 1
                print(f"{inf[2:]}有切片")
        explanation = llm_data[inf]['explanation']
        # 得到LLM认定的漏洞行
        if explanation == "" or "key_code_lines" not in explanation:
            continue
        lines = explanation.split('\"key_code_lines\":')[1].split('\"explanation\":')[0].replace('[','').replace(']','').strip()
        # print(lines)
        lines_clear = [s.replace("\"", "").replace('\\\\', '\\') for s in lines.split('\n') if s]

        # 得到原函数
        try:
            func_path = os.path.join('/home/m2024-djj/Vulrag/rag/func_text', 
                                    record[inf[2:]]['cwe'].replace('.json', ""),
                                    record[inf[2:]]['repo'],
                                    inf[2:].replace(".c", '') + '_before.c')
        except:
            func_path = os.path.join('/home/m2024-djj/Vulrag/rag/func_text', 
                                    record[inf[2:].replace("::", "__")]['cwe'].replace('.json', ""),
                                    record[inf[2:].replace("::", "__")]['repo'],
                                    inf[2:].replace("::", "__").replace(".c", '') + '_before.c')
        try:
            with open(func_path, 'r') as f:
                func_lines = f.readlines()
        except:
            with open(func_path.replace("__", "::"), 'r') as f:
                func_lines = f.readlines()
        for line_num in range(len(func_lines)):
            func_lines[line_num] = func_lines[line_num].replace('\n', '').replace('\\\\', '\\').strip()

        p = 0
        q = 0
        q_and_p = 0
        for line in lines_clear:
            if line.endswith(','):
                line = line[:-1].strip()
            if len(line) > 4:
                p += 1
            if line in func_lines:
                num = func_lines.index(line) + 1
                # print(f"Found line {line} content match in {inf[2:]}: Line {num}")
                try:
                    if num in slice_lines[inf[2:]]:
                        # print(f"{inf[2:]} -- real_llm: {slice_lines[inf[2:]]} -- pred_llm: {num}")
                        print(f"{inf[2:]}")
                        q = len(slice_lines[inf[2:]]) - 1
                        q_and_p += 1
                         
                except:
                    if num in slice_lines[inf[2:].replace("::", "__")]:
                        # print(f"{inf[2:]} -- real_llm: {slice_lines[inf[2:].replace('::', '__')]} -- pred_llm: {num}")
                        print(f"{inf[2:]}")
                        q = len(slice_lines[inf[2:].replace("::", "__")]) - 1
                        q_and_p += 1
        if p + q - q_and_p != 0:
            IoU += 1/20 * (q_and_p / (p + q - q_and_p))

    print(f"all_num: {all_num}, IoU: {IoU}")

if __name__ == "__main__":
    # get_2024_vul()
    # get_real_vul_line_2024()
    analyze_llm_explanation()
