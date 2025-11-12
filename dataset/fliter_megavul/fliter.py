import os
import json
from tqdm import tqdm  

vul = {}
novul = {}

# 读取已有的vul和novul数据
# 读取secvul
secvul_path_vul = '/home/m2024-djj/Vulrag/rag/test_data_secvul/vul.json'
with open(secvul_path_vul, 'r') as f:
    vul_data = json.load(f)
for inf in vul_data['data_2024']:
    for cve in inf['cve_list']:
        if cve not in vul:
            vul[cve] = []
        vul[cve].append(inf['func_name'])

secvul_path_novul = '/home/m2024-djj/Vulrag/rag/test_data_secvul/novul.json'
with open(secvul_path_novul, 'r') as f:
    novul_data = json.load(f)
for inf in novul_data['data_2024']:
    for cve in inf['cve_list']:
        if cve not in novul:
            novul[cve] = []
        novul[cve].append(inf['func_name'])

# 读取cvefixes
cvefixes_path_vul = '/home/m2024-djj/Vulrag/rag/test_data_secvul/fliter_cvefixes/vul'
for file in os.listdir(cvefixes_path_vul):
    isvul, cve, cwe, hash = file.split("_")[0], file.split("_")[1], file.split("_")[2], file.split("_")[3]
    len_inf = len(isvul) + len(cve) + len(cwe) + len(hash) + 4  # 4下划线
    func_name = file[len_inf:-2]  # 去掉前面的部分和后缀
    if cve not in vul:
        vul[cve] = []
    vul[cve].append(func_name)
    
cvefixes_path_novul = '/home/m2024-djj/Vulrag/rag/test_data_secvul/fliter_cvefixes/novul'
for file in os.listdir(cvefixes_path_novul):
    isvul, cve, cwe, hash = file.split("_")[0], file.split("_")[1], file.split("_")[2], file.split("_")[3]
    len_inf = len(isvul) + len(cve) + len(cwe) + len(hash) + 4  # 4下划线
    func_name = file[len_inf:-2]  # 去掉前面的部分和后缀
    if cve not in novul:
        novul[cve] = []
    novul[cve].append(func_name)


dataset_path = '/home/m2024-djj/Vulrag/rag/cwe_dir'
# 读取每个cwe
save_path_vul = '/home/m2024-djj/Vulrag/rag/test_data_secvul/fliter_megavul/vul'
save_path_novul = '/home/m2024-djj/Vulrag/rag/test_data_secvul/fliter_megavul/novul'
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
        # 处理vul
        if cve not in vul:
            with open(os.path.join(save_path_vul, '1_' + func_name+'.c'), 'a') as f:
                f.write(inf['func_before'])
            len_vul_after_fliter += 1
        elif cve in vul:
            if func_name not in vul[cve]:
                with open(os.path.join(save_path_vul, '1_' + func_name+'.c'), 'a') as f:
                    f.write(inf['func_before'])
                len_vul_after_fliter += 1
        # 处理novul
        if cve not in novul:
            with open(os.path.join(save_path_novul, '0_' + func_name +'.c'), 'a') as f:
                f.write(inf['func'])
            len_novul_after_fliter += 1
        elif cve in novul:
            if func_name not in novul[cve]:
                with open(os.path.join(save_path_novul, '0_' + func_name +'.c'), 'a') as f:
                    f.write(inf['func'])
                len_novul_after_fliter += 1
print(f"过滤megavul完成，过滤后novul:{len_novul_after_fliter}/{len_novul}, vul: {len_vul_after_fliter}/{len_vul}")