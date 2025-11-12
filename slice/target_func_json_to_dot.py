import os
import json
import pydot
import pickle

def generate_complete_json(data_nodes, complete_pdg_path, func_name):
    if complete_pdg_path[-1] != '/':
        complete_pdg_path += '/'
    #dot_path = complete_pdg_path +'1_'+ func_name + '.dot'
    dot_path = complete_pdg_path + func_name + '.dot'
    # if os.path.exists(dot_path):
    #     #print('1_'+ func_name + '.dot'+'has been processed')
    #     return
    graph = pydot.Dot(func_name, graph_type = 'digraph')
    #pdg_data_nodes = {}
    for node in data_nodes:
        node_id = '"' + data_nodes[node].id.split("id=")[-1][:-1] + '"'
        node_type = data_nodes[node].node_type
        node_code = data_nodes[node].properties.code()
        node_label = "("+node_type+','+node_code+")"
        dot_node = pydot.Node(node_id, label = node_label)
        node_edges = data_nodes[node].edges
        for node_edge in node_edges:
            node_edge_label = node_edges[node_edge].type
            if node_edge_label != 'Ast' and node_edge_label != 'Cfg':
                graph.add_node(dot_node)
                #pdg_data_nodes.update({node:data_nodes[node]})
                break
    for node in data_nodes:
        node_edges = data_nodes[node].edges
        for node_edge in node_edges:
            node_edge_label = node_edges[node_edge].type
            node_in_id = '"' + node_edges[node_edge].node_in.split("id=")[-1][:-1] + '"'
            node_out_id = '"' + node_edges[node_edge].node_out.split("id=")[-1][:-1] + '"'
            if node_edge_label == 'Ast':
                node_edge_label = 'AST: '
                continue
            elif node_edge_label == 'Cfg':
                node_edge_label = 'CFG: '
                continue
            else:
                ddg_var = node_edge.split("@")[-1]
                node_edge_label =  ddg_var
            dot_edge = pydot.Edge(node_in_id, node_out_id, label = node_edge_label)
            graph.add_edge(dot_edge)
    graph.write_raw(dot_path)
    



def generate_sub_json(all_data_nodes, _point_slice_list, func_save_path, func_name, points_name, label_path=None):
    # 处理保存路径
    # unmatched_slice = []
    idx = func_name[:]
    line_num_lists = []
    # func_name = text@pointer
    func_name += points_name
    iter = 0
    flag = 0
    # subgraph是一个startcode对应的前驱/后继节点集，如果为1，说明没有（只有本身）
    for subgraph in _point_slice_list:
        if len(subgraph) == 1:
            continue 
        edge_record = []
        node_record = []
        data_nodes = subgraph

        line_num_list = []
        # 切片内的代码行，存在line_num_list中
        for node in data_nodes: 
            line_num = node.properties.line_number()
            line_num_list.append(int(line_num))
        # print(func_name,end='')
        # print(line_num_list)

        # 切片内的代码行去重
        line_num_set = list(set(line_num_list))
        if len(line_num_set) == 1:
            continue # 排除切片内只有一行节点的切片

        # subgraph_tmp = data_nodes
        # for node in subgraph_tmp[:]:
        #     if node.label == 'MethodReturn':
        #         subgraph_tmp.remove(node)
        # if len(subgraph_tmp) == 1:
        #     continue # 排除除去头部节点只剩下一个代码行节点的切片
        
        # graph = pydot.Dot(func_name, graph_type = 'digraph')
        # for node in data_nodes:
        #     # 提取id，node_type，code，id存在node_record
        #     node_id = '"' + node.id.split("id=")[-1][:-1] + '"'
        #     node_record.append(node_id)
        #     node_type = node.node_type
        #     node_code = node.properties.code()
        #     # label = (node_type, node_code)
        #     node_label = "("+node_type+','+node_code+")"
        #     # 生成节点，添加至图中
        #     dot_node = pydot.Node(node_id, label = node_label)
        #     node_edges = node.edges
        #     graph.add_node(dot_node)
        # # 添加边
        # for node in data_nodes:
        #     node_edges = node.edges
        #     for node_edge in node_edges:
        #         node_edge_label = node_edges[node_edge].type
        #         node_in_id = '"' + node_edges[node_edge].node_in.split("id=")[-1][:-1] + '"'
        #         node_out_id = '"' + node_edges[node_edge].node_out.split("id=")[-1][:-1] + '"'
        #         if node_edge_label == 'Ast':
        #             node_edge_label = 'AST: '
        #         elif node_edge_label == 'Cfg':
        #             node_edge_label = 'CFG: '
        #         else:
        #             ddg_var = node_edge.split("@")[-1].split("#")[0]
        #             # node_edge_label = 'DDG: ' + ddg_var
        #             node_edge_label = ddg_var
        #         _edge_info = [node_in_id, node_out_id, node_edge_label]
        #         # 边信息存储在edge_record中，（node_in_id, node_out_id, node_edge_label）
        #         if _edge_info not in edge_record:
        #             edge_record.append([node_in_id, node_out_id, node_edge_label])
        # left_edge_node_list = []
        # # 遍历每条边
        # for edge_info in edge_record:
        #     # 遍历边的node_in_id和node_out_id
        #     for edge_node_id in [edge_info[0],edge_info[1]]:
        #         edge_node_id = edge_node_id[1:-1]
        #         # 如果node_id不在node_record中，添加至left_edge_node_list
        #         if edge_node_id not in node_record:
        #             left_edge_node_list.append(edge_node_id)
        # # 去重
        # left_edge_node_list = list(set(left_edge_node_list))

        # for edge_node_id in left_edge_node_list:
        #     for raw_node in all_data_nodes:
        #         raw_node_id = all_data_nodes[raw_node].id
        #         if edge_node_id in raw_node_id:
        #             node_id = '"' + edge_node_id + '"'
        #             node_record.append(node_id)
        #             node_type = all_data_nodes[raw_node].node_type
        #             node_code = all_data_nodes[raw_node].properties.code()
        #             node_label = "("+node_type+','+node_code+")"
        #             dot_node = pydot.Node(node_id, label = node_label)
        #             graph.add_node(dot_node)
        #             break
        # for edge_info in edge_record:
        #     dot_edge = pydot.Edge(edge_info[0], edge_info[1], label = edge_info[2])
        #     graph.add_edge(dot_edge)

        # dot_path = func_save_path + '/' + func_name + "#" + str(iter) + '.dot'
        # json_path = func_save_path + '/' + func_name + "#" + str(iter) + '.json'
        # if os.path.exists(dot_path):
        #     continue
        # if os.path.exists(json_path):
        #     continue
        # with open(json_path, 'w') as f:
        #     f.write(json.dumps(line_num_list))
        line_num_lists.append(line_num_list)
        # graph.write_raw(dot_path)
        # iter += 1
    return line_num_lists
