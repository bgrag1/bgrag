import os
import sys
import torch
import random 
sys.path.append('/home/m2024-djj/Vulrag/rag')
from faiss_search import *
from transformers import AutoModelForCausalLM, AutoTokenizer
from tqdm import tqdm
import pickle

# 将文件名中的特殊字符替换为下划线
def clear_name(file_name):
    return file_name.replace('::', '__').replace(' ', '_').replace('(', '_').replace(')', '_').replace('<', '_').replace('>', '_')

# 检索目标函数对应的CWE
def search_cwe(select):
    if select == '1':
        dataset = 'secvul'
        dataset_path = '/home/m2024-djj/Vulrag/rag/test_data_secvul/'
    elif select == '2':
        dataset = 'cvefixes'
        dataset_path = '/home/m2024-djj/Vulrag/rag/test_data_secvul/fliter_cvefixes/'
    elif select == '3':
        dataset = 'megavul'
        dataset_path = '/home/m2024-djj/Vulrag/rag/test_data_secvul/fliter_megavul/'
    save_path = os.path.join(dataset_path, 'retrieve_ret')
    if not os.path.exists(save_path):
        os.makedirs(save_path)

    description = get_cwe_description()
    # load_path = '/home/m2024-djj/Vulrag/rag/faiss_search/cwe_descpriptions_embeddings_fine_tuned_codebert.npy'
    # load_path = '/home/m2024-djj/Vulrag/rag/faiss_search/cwe_descpriptions_embeddings.npy'
    load_path = '/home/m2024-djj/Vulrag/rag/faiss_search/cwe_descpriptions_embeddings_fine_tuned_codebert-31.npy'
    tokenizer, model = load_llm3()
    loaded_embeddings_np = np.load(load_path)
    print(f"Embeddings 已从 {load_path} 加载成功。")

    dimension = loaded_embeddings_np.shape[1] # 获取向量维度 (768)
    
    index = faiss.IndexFlatIP(dimension)
    # 归一化    
    try:
        # 计算每个向量的 L2 范数 (长度)
        norms = np.linalg.norm(loaded_embeddings_np, axis=1, keepdims=True)
        # 处理零向量 (防止除以零) - 将长度为0的向量的范数设为极小值
        norms[norms == 0] = 1e-10
        normalized_db_embeddings_manual = loaded_embeddings_np / norms

        normalized_db_embeddings = normalized_db_embeddings_manual

    except Exception as e:
        print(f"手动 NumPy 归一化时出错: {e}")
        normalized_db_embeddings = None # 如果手动归一化也失败，则设为 None

    
    index.add(normalized_db_embeddings)
    print(f"Faiss 索引已构建，包含 {index.ntotal} 个向量。")
    # 先对vul函数进行检索
    vul_path = os.path.join(dataset_path, 'vul')
    for file_name in tqdm(os.listdir(vul_path), desc=f'为{dataset}-vul检索CWE'):
        func_name = file_name.replace('.c', '')
        save_path_file = os.path.join(save_path , 'vul')
        if not os.path.exists(save_path_file):
            os.makedirs(save_path_file)
        save_path_file = os.path.join(save_path_file, func_name)
        # if os.path.exists(save_path_file):
        #     continue
        # else:
        #     os.makedirs(save_path_file)
        func_path = os.path.join(vul_path, file_name)
        with open(func_path, 'r', encoding='utf-8') as f:
            func_text = f.read()
        # 获取查询向量
        with torch.no_grad():
            max_length = 512
            stride = max_length // 4 # 重叠步长，例如1/4，可以根据需要调整
            
            # 预先检查文本长度
            tokens_length = len(tokenizer.tokenize(func_text))
            
            # 如果文本长度未超过最大长度
            if tokens_length <= max_length:
                tokens = tokenizer(func_text, padding='max_length', return_tensors="pt", truncation=True, max_length=max_length).to(device)
            
                query_vector = model(**tokens)
                query_vector = query_vector.last_hidden_state.mean(dim=1).to('cpu')
                query_vector = query_vector.numpy().astype('float32')

                # 归一化
                norms = np.linalg.norm(query_vector, axis=1, keepdims=True)
                norms[norms == 0] = 1e-10
                query_vector = query_vector / norms

            else:
                # 使用相同的分块策略
                encoded_segments = tokenizer(
                    func_text,
                    add_special_tokens=True,
                    max_length=max_length,
                    padding='max_length',
                    truncation=True,
                    return_attention_mask=True,
                    return_token_type_ids=False,
                    return_overflowing_tokens=True,
                    stride=stride,
                    return_tensors='pt'
                ).to(device)
                # 处理每个分块
                chunk_embeddings = []
                for i in range(len(encoded_segments['input_ids'])):
                    outputs = model(
                        input_ids=encoded_segments['input_ids'][i].unsqueeze(0),
                        attention_mask=encoded_segments['attention_mask'][i].unsqueeze(0)
                    )
                    embedding = outputs.last_hidden_state.mean(dim=1).to('cpu')
                    chunk_embeddings.append(embedding)
                
                # 合并所有块的表示
                all_chunk_embeddings = torch.cat(chunk_embeddings, dim=0)
                averaged_function_embedding = torch.mean(all_chunk_embeddings, dim=0, keepdim=True).numpy().astype('float32')
                
                # 归一化
                norms = np.linalg.norm(averaged_function_embedding, axis=1, keepdims=True)
                norms[norms == 0] = 1e-10
                query_vector = averaged_function_embedding / norms

        D, I = index.search(query_vector, k=5)
        search_cwe = [description[idx][0] for idx in I[0].tolist()]
        D = D[0].tolist()
        ret = {'cwe': search_cwe, 'score': D}
        save_path_final = os.path.join(save_path_file, 'cwe_search_31.json')
        with open(save_path_final, 'w') as f:
            json.dump(ret, f, indent=4)
    # 对novul函数进行检索
    novul_path = os.path.join(dataset_path, 'novul')
    for file_name in tqdm(os.listdir(novul_path), desc=f'为{dataset}-novul检索CWE'):
        func_name = file_name.replace('.c', '')
        save_path_file = os.path.join(save_path, 'novul')
        if not os.path.exists(save_path_file):
            os.makedirs(save_path_file)
        save_path_file = os.path.join(save_path_file, func_name)
        func_path = os.path.join(novul_path, file_name)
        with open(func_path, 'r', encoding='utf-8') as f:
            func_text = f.read()
        # 获取查询向量
        with torch.no_grad():
            max_length = 512
            stride = max_length // 4 # 重叠步长，例如1/4，可以根据需要调整
            
            # 预先检查文本长度
            tokens_length = len(tokenizer.tokenize(func_text))
            
            # 如果文本长度未超过最大长度
            if tokens_length <= max_length:
                tokens = tokenizer(func_text, padding='max_length', return_tensors="pt", truncation=True, max_length=max_length).to(device)
            
                query_vector = model(**tokens)
                query_vector = query_vector.last_hidden_state.mean(dim=1).to('cpu')
                query_vector = query_vector.numpy().astype('float32')

                # 归一化
                norms = np.linalg.norm(query_vector, axis=1, keepdims=True)
                norms[norms == 0] = 1e-10
                query_vector = query_vector / norms

            else:
                # 使用相同的分块策略
                encoded_segments = tokenizer(
                    func_text,
                    add_special_tokens=True,
                    max_length=max_length,
                    padding='max_length',
                    truncation=True,
                    return_attention_mask=True,
                    return_token_type_ids=False,
                    return_overflowing_tokens=True,
                    stride=stride,
                    return_tensors='pt'
                ).to(device)
                # 处理每个分块
                chunk_embeddings = []
                for i in range(len(encoded_segments['input_ids'])):
                    outputs = model(
                        input_ids=encoded_segments['input_ids'][i].unsqueeze(0),
                        attention_mask=encoded_segments['attention_mask'][i].unsqueeze(0)
                    )
                    embedding = outputs.last_hidden_state.mean(dim=1).to('cpu')
                    chunk_embeddings.append(embedding)
                
                # 合并所有块的表示
                all_chunk_embeddings = torch.cat(chunk_embeddings, dim=0)
                averaged_function_embedding = torch.mean(all_chunk_embeddings, dim=0, keepdim=True).numpy().astype('float32')
                
                # 归一化
                norms = np.linalg.norm(averaged_function_embedding, axis=1, keepdims=True)
                norms[norms == 0] = 1e-10
                query_vector = averaged_function_embedding / norms

        D, I = index.search(query_vector, k=5)
        search_cwe = [description[idx][0] for idx in I[0].tolist()]
        D = D[0].tolist()
        ret = {'cwe': search_cwe, 'score': D}
        save_path_final = os.path.join(save_path_file, 'cwe_search_31.json')
        with open(save_path_final, 'w') as f:
            json.dump(ret, f, indent=4)

# 选取cwe中得分最高的
def fliter(all_retrieve_ret):
    sorted_data = sorted(all_retrieve_ret, key=lambda x: x['score'], reverse=True)
    return sorted_data[:1]
    # 计算snr，返回五个
    # return sorted_data[:5]

# 重排
def rerank(all_retrieve_ret):
    K = 1
    scores = []
    i = 1
    for ret in all_retrieve_ret:
        scores.append({'original_data':ret, 'cwe': ret['cwe'], 'rank1' : i, 'final_score' : 0, 'score': ret['score']})
        i += 1
    # 对分数进行排序
    scores = sorted(scores, key=lambda x: x['score'], reverse=True)
    # 获得rank2
    i = 1
    for idx in range(len(scores)):
        rank1 = scores[idx]['rank1']
        rank2 = i
        scores[idx]['original_data']['rank1'] = rank1
        scores[idx]['original_data']['rank2'] = rank2
        i += 1
        scores[idx]['final_score'] = float(1 / (K + rank1))+ float(1 / (K + rank2))
        scores[idx]['original_data']['final_score'] = scores[idx]['final_score']
    # 根据最终分数重新排序
    sorted_data = sorted(scores, key=lambda x: x['final_score'], reverse=True)
    final_data = []
    for data in sorted_data:
        final_data.append(data['original_data'])
    return final_data

# 去重
def deduplicate(data):
    temp_dict = {}
    idxs = []
    for idx, item in enumerate(data):
        file_name = item['func_name']
        score = item['score']
        if file_name not in temp_dict:
            temp_dict[file_name] = [score, idx]
        else:
            if score > temp_dict[file_name][0]:
                temp_dict[file_name] = [score, idx]
    for item in temp_dict.values():
        idxs.append(item[1])
    # 通过索引获取去重后的数据
    deduplicated_data = [data[i] for i in idxs]
    return deduplicated_data

# 获取函数描述
def get_func_description(all_retrieve_ret):
    func_description_path = '/home/m2024-djj/Vulrag/rag/abstract_description'
    for i in range(len(all_retrieve_ret)):
        cwe = all_retrieve_ret[i]['cwe']
        repo, func_name = all_retrieve_ret[i]['func_name'].split('/')[0], all_retrieve_ret[i]['func_name'].split('/')[1].replace('\n', '')
        target_path = os.path.join(func_description_path, cwe, repo, func_name + '.json')
        with open(target_path, 'r') as f:
            func_description = json.load(f)
        all_retrieve_ret[i]['description'] = func_description['description']
        all_retrieve_ret[i]['commit'] = func_description['commit']

# 接受一个切片，和一个匹配的CWE列表，返回检索到的函数
def retrieve_slice(slices, cwe, num_retrieval=2):
    # 把slice_text向量化
    ret = []
    tokenizer, model = load_llm2()
    slice_vec_path = '/home/m2024-djj/Vulrag/rag/graph/' + cwe + '/' + cwe + '_filtered_slices.pkl'
    with open(slice_vec_path, 'rb') as f:
        vectors_list = pickle.load(f)
    # print(f"Embeddings 已从 {slice_vec_path} 加载成功。")
    if vectors_list == []:
        return []
    vector_dim = len(vectors_list[0])  # 获取向量维度
    loaded_embeddings_np = np.array(vectors_list).reshape(len(vectors_list), vector_dim)
    dimension = loaded_embeddings_np.shape[1] # 768
    # 创建索引 内积检索
    index = faiss.IndexFlatIP(dimension)

    try:
        # 计算每个向量的 L2 范数 (长度)
        norms = np.linalg.norm(loaded_embeddings_np, axis=1, keepdims=True)
        # 处理零向量 (防止除以零) - 将长度为0的向量的范数设为极小值
        norms[norms == 0] = 1e-10
        normalized_db_embeddings_manual = loaded_embeddings_np / norms
        normalized_db_embeddings = normalized_db_embeddings_manual

    except Exception as e:
        print(f"手动 NumPy 归一化时出错: {e}")
        normalized_db_embeddings = None # 如果手动归一化也失败，则设为 None

    index.add(normalized_db_embeddings)
    # print(f"Faiss 索引已构建，包含 {index.ntotal} 个向量。")

    for slice_text in slices:
        with torch.no_grad():
            max_length = 512
            tokens = tokenizer(slice_text, max_length=max_length, truncation=True, return_tensors="pt").to(device)
            query_vector = model(**tokens)
            query_vector = query_vector.last_hidden_state.mean(dim=1).to('cpu')
            query_vector = query_vector.numpy().astype('float32')
            query_vector = np.array(query_vector) * 1000
            
            norms = np.linalg.norm(query_vector, axis=1, keepdims=True)
            # 处理零向量 (防止除以零) - 将长度为0的向量的范数设为极小值
            norms[norms == 0] = 1e-10
            query_vector = query_vector / norms
        D, I = index.search(query_vector, k=num_retrieval)
        # 根据检索到的行数I，从记录函数名的文件中提取函数名，同时记录相似性分数，以便Rerank
        func_name_file_path = '/home/m2024-djj/Vulrag/rag/graph/' + cwe + f'/{cwe}_func_name.txt'
        with open(func_name_file_path, 'r') as f:
            func_name_list = f.readlines()
        # todo: line是否需要加1？即是否从0开始
        for idx, line in enumerate(I[0]):
            temp_ret = {'cwe':cwe, 'func_name':func_name_list[line].replace('\n', ''), 'score':float(D[0][idx])}
            # 通过检索到的函数名，找到对应的函数
            repo, file_name = temp_ret['func_name'].split('/')[0], temp_ret['func_name'].split('/')[1].replace('\n', '')
            original_func_path = '/home/m2024-djj/Vulrag/rag/func_text/' + cwe + '/' + repo + '/' + file_name
            func_before_path = original_func_path + '_before.c'
            func_after_path = original_func_path + '_after.c'
            with open(func_before_path, 'r') as f:
                func_before = f.read()
            with open(func_after_path, 'r') as f:
                func_after = f.read()
            temp_ret['func_before'], temp_ret['func_after'] = func_before, func_after
            ret.append(temp_ret)
    ret = fliter(ret)
    return ret

# 读取输入函数的切片，进行检索(file_name带.c)
def retrieve_slices(select):
    if select == '1':
        dataset = 'secvul'
        dataset_path = '/home/m2024-djj/Vulrag/rag/test_data_secvul/'
    elif select == '2':
        dataset = 'cvefixes'
        dataset_path = '/home/m2024-djj/Vulrag/rag/test_data_secvul/fliter_cvefixes/'
    elif select == '3':
        dataset = 'megavul'
        dataset_path = '/home/m2024-djj/Vulrag/rag/test_data_secvul/fliter_megavul/'
    save_path = os.path.join(dataset_path, 'retrieve_ret')
    if not os.path.exists(save_path):
        os.makedirs(save_path)

    # 先处理vul
    slice_vul_path = os.path.join(dataset_path, 'vul')
    for func in tqdm(os.listdir(slice_vul_path), desc=f'检索{dataset}-vul函数切片'):
        all_retrieve_ret_vul = []
        # 路径处理
        func_name = func.replace('.c', '')
        slice_file = os.path.join(dataset_path, 'slice/vul', clear_name(func_name), 'slice', 'func_slice.json')
        if not os.path.exists(slice_file):
            print(f'未找到{func}的切片文件，跳过')
            continue
        save_path_1 = os.path.join(save_path, 'vul')
        if not os.path.exists(save_path_1):
            os.makedirs(save_path_1)
        save_path_2 = os.path.join(save_path_1, func_name)
        if not os.path.exists(save_path_2):
            os.makedirs(save_path_2)
        cwe_search_file = os.path.join(save_path_2, 'cwe_search_31.json')
        if not os.path.exists(cwe_search_file):
            print(f'未找到{func}的CWE检索结果，跳过')
            continue
        with open(cwe_search_file, 'r') as f:
            cwe_search = json.load(f)
        if cwe_search == {}:
            print(f'未找到{func}的CWE检索结果，跳过')
            continue
        final_save_path = os.path.join(save_path_2, 'slice_search_snr.json')
        # if os.path.exists(final_save_path):
        #     print(f'已存在{func}的检索结果，跳过')
        #     continue
        
        slices = []
        with open(slice_file, 'r') as f:
            slice_data = json.load(f)
        for type in slice_data.keys():
            # 对每个切片进行检索，保存检索到的切片和对应的函数名、分数
            if slice_data[type] == []:
                continue
            for slice in slice_data[type]:
                slices.append(slice)
        # 如果切片为空，直接返回
        if slices == []:
            continue
        # 去重
        slices = list(set(slices))
        # 对每个CWE进行检索
        for cwe in cwe_search['cwe']:
            all_retrieve_ret_vul.extend(retrieve_slice(slices, cwe))
        # 计算snr，只需要对top1的cwe进行检索
        # all_retrieve_ret_vul.extend(retrieve_slice(slices, cwe_search['cwe'][0]))
        get_func_description(all_retrieve_ret_vul)
        # 保存检索结果
        with open(final_save_path, 'w') as f:
            json.dump(all_retrieve_ret_vul, f, indent=4)
            
    # 处理novul
    slice_novul_path = os.path.join(dataset_path, 'novul')
    for func in tqdm(os.listdir(slice_novul_path), desc=f'检索{dataset}-novul函数切片'):
        # 路径处理
        all_retrieve_ret_novul = []
        func_name = func.replace('.c', '')
        slice_file = os.path.join(dataset_path, 'slice/novul', clear_name(func_name), 'slice', 'func_slice.json')
        if not os.path.exists(slice_file):
            print(f'未找到{func}的切片文件，跳过')
            continue
        save_path_1 = os.path.join(save_path, 'novul')
        if not os.path.exists(save_path_1):
            os.makedirs(save_path_1)
        save_path_2 = os.path.join(save_path_1, func_name)
        if not os.path.exists(save_path_2):
            os.makedirs(save_path_2)
        cwe_search_file = os.path.join(save_path_2, 'cwe_search_31.json')
        if not os.path.exists(cwe_search_file):
            print(f'未找到{func}的CWE检索结果，跳过')
            continue
        with open(cwe_search_file, 'r') as f:
            cwe_search = json.load(f)
        if cwe_search == {}:
            print(f'未找到{func}的CWE检索结果，跳过')
            continue
        final_save_path = os.path.join(save_path_2, 'slice_search_31.json')
        # if os.path.exists(final_save_path):
        #     print(f'已存在{func}的检索结果，跳过')
        #     continue
        
        with open(slice_file, 'r') as f:
            slice_data = json.load(f)
        slices = []
        for type in slice_data.keys():
            # 对每个切片进行检索，保存检索到的切片和对应的函数名、分数
            if slice_data[type] == []:
                continue
            for slice in slice_data[type]:
                slices.append(slice)
        # 如果切片为空，直接返回
        if slices == []:
            continue
        # 去重
        slices = list(set(slices))
        # 对每个CWE进行检索
        for cwe in cwe_search['cwe']:
            all_retrieve_ret_novul.extend(retrieve_slice(slices, cwe))
        get_func_description(all_retrieve_ret_novul)
        # 保存检索结果
        with open(final_save_path, 'w') as f:
            json.dump(all_retrieve_ret_novul, f, indent=4)

def main():
    select = input("为哪个数据集进行检索？(1: secvul, 2: cvefixes, 3: megavul)")
    if select not in ['1', '2', '3']:
        print("输入错误，请输入1、2或3。")
        return
    
    select2 = input("进行何粒度的检索？(1: CWE, 2: Slice)")

    if select2 == '1':
        search_cwe(select)
    elif select2 == '2':
        retrieve_slices(select)
    else:
        print("输入错误，请输入1或2。")
        return

if __name__ == "__main__":
    main()
