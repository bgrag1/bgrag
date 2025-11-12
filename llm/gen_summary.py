import os
import sys
import torch
import random 
import json
sys.path.append('/home/m2024-djj/Vulrag/rag')
from faiss_search import *
from sentence_transformers import SentenceTransformer
from transformers import AutoModelForCausalLM, AutoTokenizer
from tqdm import tqdm

model_name = "/home/m2024-djj/llm/model/DeepSeek-Coder-V2-Lite-Instruct"

class Model_Deepseek:
    def __init__(self, model=model_name):
        self.device = "auto"
        # self.device = 'cuda:1'
        self.model = AutoModelForCausalLM.from_pretrained(
            model,
            torch_dtype=torch.bfloat16,
            trust_remote_code=True,
            device_map="auto",
        )
        self.tokenizer = AutoTokenizer.from_pretrained(model, trust_remote_code=True)


    def generate(self, prompt):
        messages = [{"role": "user", "content": prompt}]
    
        inputs = self.tokenizer.apply_chat_template(
                messages,
                add_generation_prompt=True,
                return_tensors="pt"
            )
        inputs = inputs.to(self.model.device)
        # print(f"Len prompt: {len(prompt)}")
        outputs = self.model.generate( 
                    inputs, max_new_tokens=512, 
                    do_sample=False, 
                    top_k=50, 
                    top_p=0.95, 
                    num_return_sequences=1, 
                    eos_token_id=self.tokenizer.eos_token_id,
            )

        response = self.tokenizer.decode(outputs[0][len(inputs[0]):], skip_special_tokens=True)
        return response

# 生成prompt
def gen_prompt(code):
    return f"""//Function start
    {code}
//Function end

    What is the purpose of the function in the above code snippet? Please summarize the answer in one sentence with the following format:"Function purpose:"
"""

# 为指定数据集生成摘要
def gen_summary(dataset):
    model = Model_Deepseek()
    if dataset == '1':
        dataset_path = '/home/m2024-djj/Vulrag/rag/test_data_secvul/'
    elif dataset == '2':
        dataset_path = '/home/m2024-djj/Vulrag/rag/test_data_secvul/fliter_cvefixes/'
    elif dataset == '3':
        dataset_path = '/home/m2024-djj/Vulrag/rag/test_data_secvul/fliter_megavul/'
    # 处理文件夹
    sum_path = os.path.join(dataset_path, 'summary')
    if not os.path.exists(sum_path):
        os.makedirs(sum_path)
    sum_path_vul = os.path.join(sum_path, 'vul')
    if not os.path.exists(sum_path_vul):
        os.makedirs(sum_path_vul)
    sum_path_novul = os.path.join(sum_path, 'novul')
    if not os.path.exists(sum_path_novul):
        os.makedirs(sum_path_novul)

    func_dir_vul = os.path.join(dataset_path, 'vul')
    func_dir_novul = os.path.join(dataset_path, 'novul')

    # 为漏洞函数生成摘要
    for func in tqdm(os.listdir(func_dir_vul), desc='vul:'):
        if not func.endswith('.c'):
            continue
        with open(os.path.join(func_dir_vul, func), 'r') as f:
            code = f.read()
        prompt = gen_prompt(code)
        try:
            summary = model.generate(prompt).replace('`', '"')
        except:
            continue
        with open(os.path.join(sum_path_vul, func.replace('.c', '.txt')), 'w') as f:
            f.write(summary)
    # 为非漏洞函数生成摘要
    for func in tqdm(os.listdir(func_dir_novul), desc='novul:'):
        if not func.endswith('.c'):
            continue
        with open(os.path.join(func_dir_novul, func), 'r') as f:
            code = f.read()
        prompt = gen_prompt(code)
        try:
            summary = model.generate(prompt).replace('`', '"')
        except:
            continue
        with open(os.path.join(sum_path_novul, func.replace('.c', '.txt')), 'w') as f:
            f.write(summary)

def retrieve_cwe_codebert(description, index, summary, tokenizer, model):
    # 得到查询向量
    with torch.no_grad():
        tokens = tokenizer(summary, padding='max_length', return_tensors="pt", truncation=True, max_length=512).to(device)

        query_vector = model(**tokens)
        query_vector = query_vector.last_hidden_state.mean(dim=1).to('cpu')
        query_vector = query_vector.numpy().astype('float32')

        # 归一化
        norms = np.linalg.norm(query_vector, axis=1, keepdims=True)
        norms[norms == 0] = 1e-10
        query_vector = query_vector / norms

    D, I = index.search(query_vector, k=5)
    search_cwe = [description[idx][0] for idx in I[0].tolist()]
    D = D[0].tolist()
    return {'cwe': search_cwe, 'score': D}

# 为指定数据集检索CWE（codebert）
def retrieve_cwes_codebert(dataset):
    # 由codebert生成索引
    description = get_cwe_description()
    load_path = '/home/m2024-djj/Vulrag/rag/faiss_search/cwe_descpriptions_embeddings_codebert-base.npy'
    tokenizer, model = load_llm2()
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
    # 数据集
    if dataset == '1':
        dataset_path = '/home/m2024-djj/Vulrag/rag/test_data_secvul/'
    elif dataset == '2':
        dataset_path = '/home/m2024-djj/Vulrag/rag/test_data_secvul/fliter_cvefixes/'
    elif dataset == '3':
        dataset_path = '/home/m2024-djj/Vulrag/rag/test_data_secvul/fliter_megavul/'
    save_path = os.path.join(dataset_path, 'retrieve_ret')
    summary_path = os.path.join(dataset_path, 'summary')
    # 处理vul
    for func in tqdm(os.listdir(os.path.join(summary_path, 'vul')), desc='vul:'):
        if not func.endswith('.txt'):
            continue
        with open(os.path.join(summary_path, 'vul', func), 'r') as f:
            summary = f.read()
        summary = summary.split('Function purpose:')[-1].strip()
        # 检索
        ret = retrieve_cwe_codebert(description, index, summary, tokenizer, model)
        final_save_path = os.path.join(save_path, 'vul', func.replace('.txt', ''), 'cwe_summary.json')
        with open(final_save_path, 'w') as f:
            json.dump(ret, f, indent=4)
    # 处理novul
    for func in tqdm(os.listdir(os.path.join(summary_path, 'novul')), desc='novul:'):
        if not func.endswith('.txt'):
            continue
        with open(os.path.join(summary_path, 'novul', func), 'r') as f:
            summary = f.read()
        summary = summary.split('Function purpose:')[-1].strip()
        # 检索
        ret = retrieve_cwe_codebert(description, index, summary, tokenizer, model)
        final_save_path = os.path.join(save_path, 'novul', func.replace('.txt', ''), 'cwe_summary.json')
        with open(final_save_path, 'w') as f:
            json.dump(ret, f, indent=4)

# 为指定数据集检索CWE（sentence-transformers）
def retrieve_cwes_sentence(dataset):
    # 由sentence-transformers生成索引
    description = get_cwe_description()
    load_path = '/home/m2024-djj/Vulrag/rag/llm/cwe_descriptions_embeddings_sentence.npy'
    model = SentenceTransformer('/home/m2024-djj/llm/model/sentence-transformers')
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
        normalized_db_embeddings = loaded_embeddings_np / norms

    except Exception as e:
        print(f"手动 NumPy 归一化时出错: {e}")
        normalized_db_embeddings = None # 如果手动归一化也失败，则设为 None

    index.add(normalized_db_embeddings)
    print(f"Faiss 索引已构建，包含 {index.ntotal} 个向量。")
    # 数据集
    if dataset == '1':
        dataset_path = '/home/m2024-djj/Vulrag/rag/test_data_secvul/'
    elif dataset == '2':
        dataset_path = '/home/m2024-djj/Vulrag/rag/test_data_secvul/fliter_cvefixes/'
    elif dataset == '3':
        dataset_path = '/home/m2024-djj/Vulrag/rag/test_data_secvul/fliter_megavul/'
    save_path = os.path.join(dataset_path, 'retrieve_ret')
    summary_path = os.path.join(dataset_path, 'summary')
    # 处理vul
    for func in tqdm(os.listdir(os.path.join(summary_path, 'vul')), desc='vul:'):
        if not func.endswith('.txt'):
            continue
        with open(os.path.join(summary_path, 'vul', func), 'r') as f:
            summary = f.read()
        summary = summary.split('Function purpose:')[-1].strip()
        # 检索
        # 生成embedding并归一化
        query_vector = model.encode([summary])
        query_vector = query_vector / (np.linalg.norm(query_vector, axis=1, keepdims=True) + 1e-10)
        D, I = index.search(query_vector.astype('float32'), k=5)
        search_cwe = [description[idx][0] for idx in I[0].tolist()]
        D = D[0].tolist()
        ret = {'cwe': search_cwe, 'score': D}
        final_save_path = os.path.join(save_path, 'vul', func.replace('.txt', ''), 'cwe_summary.json')
        os.makedirs(os.path.dirname(final_save_path), exist_ok=True)
        with open(final_save_path, 'w') as f:
            json.dump(ret, f, indent=4)

    # 处理novul
    for func in tqdm(os.listdir(os.path.join(summary_path, 'novul')), desc='novul:'):
        if not func.endswith('.txt'):
            continue
        with open(os.path.join(summary_path, 'novul', func), 'r') as f:
            summary = f.read()
        summary = summary.split('Function purpose:')[-1].strip()
        # 检索
        query_vector = model.encode([summary])
        query_vector = query_vector / (np.linalg.norm(query_vector, axis=1, keepdims=True) + 1e-10)
        D, I = index.search(query_vector.astype('float32'), k=5)
        search_cwe = [description[idx][0] for idx in I[0].tolist()]
        D = D[0].tolist()
        ret = {'cwe': search_cwe, 'score': D}
        final_save_path = os.path.join(save_path, 'novul', func.replace('.txt', ''), 'cwe_summary.json')
        os.makedirs(os.path.dirname(final_save_path), exist_ok=True)
        with open(final_save_path, 'w') as f:
            json.dump(ret, f, indent=4)


def main():
    choice = input("1 for generate summary, 2 for retrieve cwe:")
    if choice == '1':
        select = input("Use which dataset?(1. secvul, 2. cvefixes, 3. megavul): ")
        if select != '1' and select != '2' and select != '3' and select != '123':
            print("Invalid selection.")
            return
        if select == '123':
            gen_summary('1')
            gen_summary('2')
            gen_summary('3')
        else:
            gen_summary(select)

    elif choice == '2':
        select = input("Use which dataset?(1. secvul, 2. cvefixes, 3. megavul): ")
        if select != '1' and select != '2' and select != '3' and select != '123':
            print("Invalid selection.")
            return
        if select == '123':
            retrieve_cwes_sentence('1')
            retrieve_cwes_sentence('2')
            retrieve_cwes_sentence('3')
        else:
            retrieve_cwes_sentence(select)

    else:
        print("Invalid choice. Please enter 1 or 2.")
        return

if __name__ == "__main__":
    main()