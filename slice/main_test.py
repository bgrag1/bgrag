import os
from re import I
from typing import Container
from preprocess import joern_process, parse_to_nodes
from preprocess import *
from complete_pdg import *
from slice_op import *
from json_to_dot import *
import argparse
from tqdm import tqdm
import logging

logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - %(name)s - %(levelname)s - %(message)s',
    handlers=[
        logging.FileHandler('generate_slice.log'),
        logging.StreamHandler()
    ]
)
logger = logging.getLogger(__name__)

def process_idx_special(idx):
    idx = idx.replace(' ', '').replace('<', '_').replace('>', '_')
    return idx

def main():
    # json_file = '/home/pc/dataset/big-vul/3_export/vul_new/'
    # pdg_dot_file = '/home/pc/dataset/big-vul/3_export/vul_pdg/'
    json_file = '/home/m2024-djj/Vulrag/rag/output_bin'
    pdg_dot_file = '/home/m2024-djj/Vulrag/rag/output_bin'

    diff_line_file = '/home/m2024-djj/Vulrag/rag/diff_lines'

    # sub_graph_path = '/home/pc/dataset/big-vul/4_slices/vul/'
    # label_path = '/home/pc/dataset/big-vul/test_label_pkl1.pkl'
    # dict_path='/home/pc/dataset/big-vul/6_line2node/vul/'
    outout_slice_path = '/home/m2024-djj/Vulrag/rag/output_slice/'
    output_dict_path = '/home/m2024-djj/Vulrag/rag/output_dict/'

    # 返回值container是一个列表，遍历所有json文件，每个元素是一个元组，元组的第一个元素是idx，
    # 第二个元素是存储了'file'键值对不为'N/A'的函数信息的字典，且'file'键值对不为'N/A'
    # 然后删除了'file'键值对, idx就是file的文件名
    # 函数信息字典有'function'、'id'、'cfg'、'ast'、'pdg'
    # container: [ [ (idx, { functions:{function, id, cfg, ast, pdg} }) ], [], ...]
    for cwe in tqdm(os.listdir(json_file)):
        if cwe != 'CWE-369':
            continue
        if cwe.endswith('.txt'):
            continue
        i = 0
        # 生成用于保存的cwe文件夹-dot
        cwe_save_to_dot = os.path.join(outout_slice_path, cwe)
        if not os.path.exists(cwe_save_to_dot):
            os.makedirs(cwe_save_to_dot)
        # 生成用于保存的cwe文件夹-json
        cwe_save_to_json = os.path.join(output_dict_path, cwe)
        if not os.path.exists(cwe_save_to_json):
            os.makedirs(cwe_save_to_json)
        # 读取已经完成过的
        record_txt = os.path.join(cwe_save_to_json, 'record.txt') 
        if not os.path.exists(record_txt):
            os.system("touch "+record_txt)
        with open(record_txt,'r') as f:
            rec_list = f.readlines()


        for repo in os.listdir(os.path.join(json_file, cwe)):
            if repo != 'Exiv2':
                continue
            if repo.endswith('.txt'):
                continue
            # 生成用于保存的repo文件夹-dot
            repo_save_to_dot = os.path.join(outout_slice_path, cwe, repo)
            if not os.path.exists(repo_save_to_dot):
                os.makedirs(repo_save_to_dot)
            # 生成用于保存的repo文件夹-json
            repo_save_to_json = os.path.join(output_dict_path, cwe, repo)
            if not os.path.exists(repo_save_to_json):
                os.makedirs(repo_save_to_json)

            # 读取json+dot文件
            load_json_file = os.path.join(json_file, cwe, repo)
            load_pdg_dot_file = os.path.join(pdg_dot_file, cwe, repo)
            # 读取diff_line信息
            diff_line_json_file = os.path.join(diff_line_file, cwe + '.', repo)
            # 读取仓库下的所有json文件
            container = joern_process(load_json_file)

            sub_cnt = 0

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
                idx = process_idx_special(data[0])
                # cpg = { functions:{function, id, cfg, ast, pdg} }
                cpg = data[1]
                # 如果处理过，跳过
                if repo + '/' + idx + '\n' in rec_list:
                    continue

                if '_after' in idx:
                    process_type = 'after'
                    # eg: func_json = ValidataSinature.json
                    func_json = idx.replace('_after', '.json')
                elif '_before' in data[0]:
                    process_type = 'before'
                    func_json = idx.replace('_before', '.json') 
                try:
                    with open(os.path.join(diff_line_json_file, func_json), 'r', encoding='utf-8') as f:
                        diff_lines = json.load(f)
                except Exception as e:
                    with open('/home/m2024-djj/Vulrag/rag/fail_generate_slice.txt','a') as f:
                        f.write(f'{repo_save_to_json}/{idx}    Error::{str(e)}' + '\n')
                if process_type == 'after':
                    traget_lines = diff_lines['llm_added_lines']
                elif process_type == 'before':
                    traget_lines = diff_lines['llm_deleted_lines']

                func_save_to_dot = os.path.join(repo_save_to_dot, idx)
                func_save_to_json = os.path.join(repo_save_to_json, idx)
                if not os.path.exists(func_save_to_dot):
                    os.makedirs(func_save_to_dot)
                if not os.path.exists(func_save_to_json):  
                    os.makedirs(func_save_to_json)

                print(f"===========>>>>>  {str(i)} : {idx}")

                if traget_lines == []:
                    with open(os.path.join(func_save_to_json, 'ret.txt'), 'a') as f:
                        f.write('Empty traget lines')
                    continue
                # ddg_edge_genearate()读取.dot文件，返回边列表(pdg), 读取的文件是pdg_dot_file+idx+'.dot'
                pdg_edge_list = ddg_edge_genearate(load_pdg_dot_file, idx)
                if pdg_edge_list == []:
                    continue
                # 返回ast的节点,过滤筛选有code和line_number的节点（大概？）
                try:
                    data_nodes_tmp = parse_to_nodes(cpg)
                except:
                    logging.warning(f"{cwe}-{repo}-{idx} :: parse_to_nodes error")
                    continue
                # 节点+边
                # 把边添加到节点
                try:
                    data_nodes = complete_pdg(data_nodes_tmp, pdg_edge_list)
                except:
                    logging.warning(f"{cwe}-{repo}-{idx} :: complete_pdg error")                    
                    continue
                # 从data_nodes筛选出pointer类型的节点
                try:
                    pointer_node_list = get_pointers_node(data_nodes)
                    if pointer_node_list != []:
                        # _pointer_slice_list存的是对pointer节点切片的节点集[ [node1, ...], [], ...]
                        _pointer_slice_list = pointer_slice(data_nodes, pointer_node_list)
                        points_name = '@pointer'
                        generate_sub_json(traget_lines, data_nodes, _pointer_slice_list, func_save_to_dot, func_save_to_json, idx, points_name, label_path=None)

                    arr_node_list = get_all_array(data_nodes)
                    if arr_node_list != []:
                        _arr_slice_list = array_slice(data_nodes, arr_node_list)
                        points_name = '@array'
                        generate_sub_json(traget_lines, data_nodes, _arr_slice_list, func_save_to_dot, func_save_to_json, idx, points_name, label_path=None)

                    integer_node_list = get_all_integeroverflow_point(data_nodes)
                    if integer_node_list != []:
                        _integer_slice_list = inte_slice(data_nodes, integer_node_list)
                        points_name = '@integer'
                        generate_sub_json(traget_lines, data_nodes, _integer_slice_list, func_save_to_dot, func_save_to_json, idx, points_name, label_path=None)

                    call_node_list = get_all_sensitiveAPI(data_nodes)
                    if call_node_list != []:
                        _call_slice_list = call_slice(data_nodes, call_node_list)
                        points_name = '@API'
                        generate_sub_json(traget_lines, data_nodes, _call_slice_list, func_save_to_dot, func_save_to_json, idx, points_name, label_path=None)          
                except Exception as e:
                    with open('/home/m2024-djj/Vulrag/rag/fail_generate_slice.txt','a') as f:
                        f.write(f'{repo_save_to_json}/{idx}    (main)Error::{str(e)}' + '\n')
                # 处理完一个函数, 记录函数信息
                with open(record_txt, 'a') as f:
                    f.write(repo + '/' + idx + '\n')
        
        with open('/home/m2024-djj/Vulrag/rag/slice/finish_cwe.txt', 'a') as f:
            f.write(cwe + '\n')

if __name__ == '__main__':
    main()
