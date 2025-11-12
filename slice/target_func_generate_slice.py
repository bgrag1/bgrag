import os
from re import I
from typing import Container
from .preprocess import joern_process, parse_to_nodes
from complete_pdg import *
from slice_op import *
from target_func_json_to_dot import *
import sys
sys.path.append('/home/m2024-djj/Vulrag/rag')
from normalization.normalization import *

from tqdm import tqdm

# 通过行数得到具体代码行
def get_code_line_old(slice_data, file_name):
    vul = 0
    if file_name.split('_')[0] == '1':
        vul = 1
    file_path = '/home/m2024-djj/Vulrag/rag/test_data' + ('/vul' if vul else '/novul_2') 
    file_path = os.path.join(file_path, file_name+'.c')
    with open(file_path, 'r', encoding='utf-8') as f:
        func_text = f.read().split('\n')
    code_lines = {}
    for type in slice_data.keys():
        code_lines[type] = []
        for slice in slice_data[type]:
            slice_text = []
            slice = sorted(list(set(slice)))
            for line in slice:
                slice_text.append(func_text[line - 1])
            if slice_text not in code_lines[type]:
                #对slice进行标准化
                slice_text = clean_gadget(slice_text)
                for idx in range(len(slice_text)):
                    slice_text[idx] = remove_duplicated_blank(slice_text[idx])
                modify_slice_text = '\n'.join(slice_text)
                code_lines[type].append(modify_slice_text)
    return code_lines

def get_code_line(slice_data, dataset, file_name, isvul):
    if dataset == 'secvul':
        file_path = '/home/m2024-djj/Vulrag/rag/test_data_secvul' + ('/vul' if isvul else '/novul') 
    elif dataset == 'cvefixes':
        file_path = '/home/m2024-djj/Vulrag/rag/test_data_secvul/fliter_cvefixes' + ('/vul' if isvul else '/novul')
    elif dataset == 'megavul':
        file_path = '/home/m2024-djj/Vulrag/rag/test_data_secvul/fliter_megavul' + ('/vul' if isvul else '/novul')
    file_path = os.path.join(file_path, file_name+'.c')
    with open(file_path, 'r', encoding='utf-8') as f:
        func_text = f.read().split('\n')
    code_lines = {}
    for type in slice_data.keys():
        code_lines[type] = []
        for slice in slice_data[type]:
            slice_text = []
            slice = sorted(list(set(slice)))
            for line in slice:
                slice_text.append(func_text[line - 1])
            if slice_text not in code_lines[type]:
                #对slice进行标准化
                slice_text = clean_gadget(slice_text)
                for idx in range(len(slice_text)):
                    slice_text[idx] = remove_duplicated_blank(slice_text[idx])
                modify_slice_text = '\n'.join(slice_text)
                code_lines[type].append(modify_slice_text)
    return code_lines

def process_idx_special(idx):
    idx = idx.replace(' ', '').replace('<', '_').replace('>', '_').replace('::','__')
    return idx

def slice(save_path, dataset, isvul):
    save_path_slice = os.path.join(save_path, 'slice')
    if not os.path.exists(save_path_slice):
        os.makedirs(save_path_slice)
    # 读取json+dot文件
    container = joern_process(save_path)
    sub_cnt = 0
    i = 0
    # data = [ (idx, { functions:{function, id, cfg, ast, pdg} }) ]
    for data in container:
        i += 1
        if data == []:
            print(f"===========>>>>>  {str(i)} : {idx}  ----- data empty")
            sub_cnt += 1
            continue
        # 加载diff信息
        # data = (idx, { functions:{function, id, cfg, ast, pdg} })
        data = data[0]
        data_nodes = {}
        # eg: idx = ValidataSinature_before
        func_name = data[0]
        idx = process_idx_special(data[0])
        # cpg = { functions:{function, id, cfg, ast, pdg} }
        cpg = data[1]

        print(f"===========>>>>>  {str(i)} : {idx}")

        # ddg_edge_genearate()读取.dot文件，返回边列表(pdg), 读取的文件是pdg_dot_file+idx+'.dot'
        pdg_edge_list = ddg_edge_genearate(save_path, idx)
        if pdg_edge_list == []:
            continue
        # 返回ast的节点,过滤筛选有code和line_number的节点（大概？）
        data_nodes_tmp = parse_to_nodes(cpg)
        # 节点+边
        # 把边添加到节点
        data_nodes = complete_pdg(data_nodes_tmp, pdg_edge_list)
        # 从data_nodes筛选出pointer类型的节点
        slice_data = {}
        try:
            pointer_node_list = get_pointers_node(data_nodes)
            if pointer_node_list != []:
                # _pointer_slice_list存的是对pointer节点切片的节点集[ [node1, ...], [], ...]
                _pointer_slice_list = pointer_slice(data_nodes, pointer_node_list)
                points_name = '@pointer'
                slice_data['pointer'] = generate_sub_json(data_nodes, _pointer_slice_list, save_path_slice, idx, points_name, label_path=None)

            arr_node_list = get_all_array(data_nodes)
            if arr_node_list != []:
                _arr_slice_list = array_slice(data_nodes, arr_node_list)
                points_name = '@array'
                slice_data['array'] = generate_sub_json(data_nodes, _arr_slice_list, save_path_slice, idx, points_name, label_path=None)

            integer_node_list = get_all_integeroverflow_point(data_nodes)
            if integer_node_list != []:
                _integer_slice_list = inte_slice(data_nodes, integer_node_list)
                points_name = '@integer'
                slice_data['integer'] = generate_sub_json(data_nodes, _integer_slice_list, save_path_slice, idx, points_name, label_path=None)

            call_node_list = get_all_sensitiveAPI(data_nodes)
            if call_node_list != []:
                _call_slice_list = call_slice(data_nodes, call_node_list)
                points_name = '@API'
                slice_data['API'] = generate_sub_json(data_nodes, _call_slice_list, save_path_slice, idx, points_name, label_path=None)          
        except Exception as e:
            with open(os.path.join(save_path_slice, 'fail.txt'), 'a') as f:
                f.write(f"Error processing {idx}: {str(e)}\n")
            return 
        final_data = get_code_line(slice_data, dataset, func_name, isvul)
        with open(os.path.join(save_path_slice, 'func_slice.json'), 'w') as f:
            json.dump(final_data, f, indent=4)
        # 处理完一个函数, 记录函数信息
        # with open(record_txt, 'a') as f:
        #     f.write(repo + '/' + idx + '\n')

def main():
    return 

if __name__ == '__main__':
    main()
