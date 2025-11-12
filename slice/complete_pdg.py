import os
import json
import pydot
import glob
from utils_dataset.objects.cpg.edge import Edge
import logging


def ddg_edge_genearate(ddg_dot_path, idx):
    if ddg_dot_path[-1] != '/':
        ddg_dot_path += '/'
    dot=ddg_dot_path+ idx +'.dot'
    try:
        graph = pydot.graph_from_dot_file(dot)[0]
        ddg_edge_list = graph.get_edges()
    except Exception as e:
        with open('/home/m2024-djj/Vulrag/rag/fail_generate_slice.txt','a') as f:
            f.write(f'{ddg_dot_path}{idx}    Error::{str(e)}' + '\n')
        return []
    return ddg_edge_list


def complete_pdg(data_nodes_tmp, ddg_edge_list):
    if type(ddg_edge_list) == bool:
        return data_nodes_tmp

    node_id_dict = {}
    for node in data_nodes_tmp:
        node_id = data_nodes_tmp[node].id.split('id=')[-1][:-1]
        node_id_dict[node_id] = data_nodes_tmp[node].id
    for dot_edge in ddg_edge_list:
        node_in = None
        node_out = None
        src_node_id_tmp = dot_edge.get_source()[1:-1]
        dst_node_id_tmp  = dot_edge.get_destination()[1:-1]
        try:
            dot_edge_attr = dot_edge.obj_dict['attributes']['label'][1:-1]
        except:
            dot_edge_attr = '$head'
            continue
        for node_id in node_id_dict.keys():
            if src_node_id_tmp == node_id:
                node_in = node_id_dict[node_id]

            elif dst_node_id_tmp == node_id:
                node_out = node_id_dict[node_id]

        if node_in == None or node_out == None:
            continue

        edge_tmp = {}
        if dot_edge_attr.find('DDG') != -1:
            ddg_edge_name = 'Ddg@'+ dot_edge_attr
        else:
            ddg_edge_name = 'Cdg@'+ dot_edge_attr
        ddg_edge_name_tmp = ddg_edge_name
        edge_tmp['id'] = ddg_edge_name
        edge_tmp['in'] = node_in
        edge_tmp['out'] = node_out
        edge = Edge(edge_tmp,indentation=3)

        cnt = 1
        while (ddg_edge_name in data_nodes_tmp[node_in].edges.keys()):
            ddg_edge_name = ddg_edge_name.split('#')[0] + '#' + str(cnt)
            cnt += 1
        data_nodes_tmp[node_in].edges[ddg_edge_name] = edge
        
        while (ddg_edge_name_tmp in data_nodes_tmp[node_out].edges.keys()):
            ddg_edge_name_tmp = ddg_edge_name_tmp.split('#')[0] + '#' + str(cnt)
            cnt += 1
        ddg_edge_name = ddg_edge_name_tmp
        data_nodes_tmp[node_out].edges[ddg_edge_name] = edge

    return data_nodes_tmp

