import json
import os

# 处理vul
file_vul = '/home/m2024-djj/Vulrag/rag/test_data_secvul/vul.json'
with open(file_vul, 'r') as f:
    vul_data = json.load(f)
save_path_vul = '/home/m2024-djj/Vulrag/rag/test_data_secvul/vul'
if not os.path.exists(save_path_vul):
    os.makedirs(save_path_vul)
for item in vul_data['data_2024']:
    func = item['func_body']
    file_name = item['func_name']
    i = 1
    while 1:
        if not os.path.exists(f"{save_path_vul}/{file_name}.c"):
            break
        # 如果文件名已存在，添加后缀数字
        file_name = f"{file_name}_{i}"
        i += 1
    with open(f"{save_path_vul}/{file_name}.c", 'w') as f:
        f.write(func)
all_file_vul = os.listdir(save_path_vul)
print(len(all_file_vul))


# 处理novul
file_novul = '/home/m2024-djj/Vulrag/rag/test_data_secvul/novul.json'
with open(file_novul, 'r') as f:
    novul_data = json.load(f)
save_path_novul = '/home/m2024-djj/Vulrag/rag/test_data_secvul/novul'
if not os.path.exists(save_path_novul):
    os.makedirs(save_path_novul)
for item in novul_data['data_2024']:
    func = item['func_body']
    file_name = item['func_name']
    i = 1
    while 1:
        if not os.path.exists(f"{save_path_novul}/{file_name}.c"):
            break
        # 如果文件名已存在，添加后缀数字
        file_name = f"{file_name}_{i}"
        i += 1
    with open(f"{save_path_novul}/{file_name}.c", 'w') as f:
        f.write(func)
all_file_novul = os.listdir(save_path_novul)
print(len(all_file_novul))