# BGRAG

We propose and implement the BGRAG framework, which achieves high relevance and low noise through CWE-level knowledge organization and a two-stage retrieval mechanism.

# dataset
The dataset we used is as follows.

`MegaVul`: https://github.com/Icyrockton/MegaVul

`CVEfixes` : https://github.com/secureIT-project/CVEfixes

`SecVulEval` : https://github.com/basimbd/SecVulEval


## enivorment
torch 2.3.0

Joern 1.1.172. You can find it in Joern's historical releases: https://github.com/joernio/joern

python 3.9.2


# 1.Vulnerability Metadata Preprocessing

normalization:

`python normalization/normalization.py`

**（1）text-level metadata preprocessing:**

`python llm/abstract_description.py`

**（2）code-level metadata preprocessing**

`python get_line_llm.py`


# 2.CWE-oriented Core Behavior Extraction

**（1）function slice:**
- input :the normalized code

- output: the .bin file
```
  python slice/joern_graph_gen.py -t pasre

```
- input: the .bin file

- output: the folder of pdg.dot of function
```
  python slice/joern_graph_gen.py -t export -r pdg
```
- input: the folder of .bin file

- output: the folder of line_info.json
```
 python slice/joern_graph_gen.py -t export -r lineinfo_json
```
- input: line_info.json, pdg.dot, and the pickle file of deletions

- output: the folder of pdg.dot of slice
```
  python prepreprocess/slice_process/main.py
```



**（2）behavior graph construct**

  python cluster/slice_embed_test_t.py



**（3）Clustering**

python cluster/graph_gen_edges_t.py



# 3.Retrieval

**（1） function-level retrieval**

generate index of cwe description
```
python llm/embedding_cwe_desc.py
```
retrieve cwe
```
python retrieve.py (dataset) (cwe)
```


**（2）Slice-level retrieval**
```
python retrieve.py (dataset) (slice)
```

# Vulerability detecion and explantion

input: (1)target function source code (2)target function retrieve_result
```
python llm/run_llm.py
```
