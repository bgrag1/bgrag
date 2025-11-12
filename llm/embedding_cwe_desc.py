import json
import numpy as np
import torch
from sentence_transformers import SentenceTransformer


device = "cuda:0" 
model_path = '/home/m2024-djj/llm/model/sentence-transformers'
model = SentenceTransformer(model_path)


def get_cwe_description():
    with open('/home/m2024-djj/Vulrag/rag/get_cwe_description/cwe_description_drop.json', 'r') as f:
        cwe_description = json.load(f)
    description = []
    for key, value in cwe_description.items():
        description.append([key, value['description']])
    return description

def get_embedding_sentence():
    descriptions = get_cwe_description()
    description_text = [desc[1] for desc in descriptions]
    embeddings = model.encode(description_text)
    print("最终整合后的 Tensor Shape:", embeddings.shape)
    save_path = '/home/m2024-djj/Vulrag/rag/llm/cwe_descriptions_embeddings_sentence'
    try:
        np.save(save_path, embeddings)
        print(f"Embeddings 已成功保存到: {save_path}")
    except Exception as e:
        print(f"保存 Embeddings 时出错: {e}")

if __name__ == "__main__":
    get_embedding_sentence()
