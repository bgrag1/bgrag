import os
import json
import pickle
import torch
import gc
from transformers import RobertaTokenizer, RobertaModel
from torch.utils.data import DataLoader
from collections import defaultdict

# 加载模型
device = torch.device("cuda:0" if torch.cuda.is_available() else "cpu")
tokenizer = RobertaTokenizer.from_pretrained("/home/m2024-djj/llm/model/codebert-base")
model = RobertaModel.from_pretrained("/home/m2024-djj/llm/model/codebert-base")
model = model.to(device)

# 关键改进：全局跨CWE过滤
def load_test_entries():
    """ 加载所有测试条目到全局集合（忽略CWE分类） """
    test_files = [
        "/home/m2024-djj/Vulrag/rag/test_data/test/range5-10/all_range5-10_generated.json",
        "/home/m2024-djj/Vulrag/rag/test_data/test/gt10/all_gt10_generated.json"
    ]
    
    # 全局集合：包含所有测试条目（不区分CWE）
    global_test_entries = set()
    
    for test_file in test_files:
        if not os.path.exists(test_file):
            print(f"⚠️ 测试索引文件缺失: {test_file}")
            continue
            
        try:
            with open(test_file, 'r') as f:
                test_data = json.load(f)
                
            # 合并所有CWE的条目
            for entries in test_data.values():
                global_test_entries.update(entries)
                
        except Exception as e:
            print(f"加载测试文件 {test_file} 失败: {str(e)}")
    
    print(f"已加载 {len(global_test_entries)} 个跨CWE测试条目")
    return global_test_entries

def embed_one_batch(codes, max_length=256):
    inputs = tokenizer(
        codes, padding=True, truncation=True, 
        return_tensors="pt", max_length=max_length
    ).to(device)
    with torch.no_grad():
        return model(**inputs)

def mycollate_fn_for_embed(data):
    codes = [item["slice"] for item in data]
    files = [item["file"] for item in data]
    return codes, files, len(files)

def embed_all(data, dataloader):
    all_embeds = defaultdict(list)
    
    for batch in dataloader:
        codes, files, length = batch
        modify_codes = ['\n'.join(code) for code in codes]
        
        outputs = embed_one_batch(modify_codes)
        embeddings = outputs.last_hidden_state[:, 0, :].cpu().numpy()
        
        for i in range(length):
            all_embeds[files[i]].append(embeddings[i])
    
    return all_embeds

if __name__ == "__main__":
    # 初始化全局过滤器
    global_test_entries = load_test_entries()
    
    # 配置处理任务
    tasks = [
        {
            "sample_index": "/home/m2024-djj/Vulrag/rag/test_data/sample/range5-10/all_range5-10_generated.json",
            "save_dir": "/home/m2024-djj/Vulrag/rag/slice_vec/range5-10"
        },
        {
            "sample_index": "/home/m2024-djj/Vulrag/rag/test_data/sample/gt10/all_gt10_generated.json",
            "save_dir": "/home/m2024-djj/Vulrag/rag/slice_vec/gt10"
        }
    ]
    
    dataset_base_path = "/home/m2024-djj/Vulrag/rag/output_slice_text"
    batch_size = 12
    
    for task in tasks:
        sample_index = task["sample_index"]
        save_dir = task["save_dir"]
        os.makedirs(save_dir, exist_ok=True)
        
        if not os.path.exists(sample_index):
            print(f"❌ 样本索引文件缺失: {sample_index}")
            continue
            
        with open(sample_index, 'r') as f:
            index_data = json.load(f)
        
        for cwe, repo_func_list in index_data.items():
            # 关键过滤逻辑：全局匹配
            filtered_repo_funcs = [
                rf for rf in repo_func_list 
                if rf not in global_test_entries
            ]
            
            print(f"[{cwe}] 原始条目: {len(repo_func_list)}, 过滤后: {len(filtered_repo_funcs)}")
            
            # 数据加载与处理
            dataset = []
            for repo_func in filtered_repo_funcs:
                try:
                    repo, func_name = repo_func.split('/', 1)
                    func_path = os.path.join(
                        dataset_base_path, cwe, repo, 
                        f"{func_name}_before.json"
                    )
                    
                    if os.path.exists(func_path):
                        with open(func_path, 'r') as f:
                            slices = json.load(f)
                            
                        dataset.append({
                            "file": repo_func,
                            "slices": slices
                        })
                except Exception as e:
                    print(f"处理 {repo_func} 失败: {str(e)}")
            
            if not dataset:
                print(f"⚠️ 无有效数据: {cwe}")
                continue
            
            # 创建DataLoader
            slices_dataset = [
                {"file": item["file"], "slice": slice_data}
                for item in dataset for slice_data in item["slices"]
            ]
            dataloader = DataLoader(
                slices_dataset, 
                batch_size=batch_size, 
                collate_fn=mycollate_fn_for_embed
            )
            
            # 生成嵌入
            embeds = embed_all(dataset, dataloader)
            
            # 保存结果
            for item in dataset:
                item["slices_vec"] = embeds[item["file"]]
            
            save_path = os.path.join(save_dir, f"{cwe}.pkl")
            with open(save_path, "wb") as f:
                pickle.dump(dataset, f)
            
            print(f"✅ 已保存 {cwe} 的嵌入结果")