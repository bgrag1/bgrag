import json
import os
import logging

# 查找某行函数在原函数中的行号

logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - %(name)s - %(levelname)s - %(message)s',
    handlers=[
        logging.FileHandler('vulrag.log'),
        logging.StreamHandler()
    ]
)
logger = logging.getLogger(__name__)

def remove_special_chars(text):
    special_chars = [' ', '\\', '\t', '`', '*', '}', '{', '(', ')', '[', ']', '#', '|', ';', ',', '/']
    result = text
    for char in special_chars:
        result = result.replace(char, '')
    return result

# 用于处理megavul数据集中的func/func_before中多出的换行符,但added_line和deleted_line中没有换行符
def handle_line_break(func_line, option_line_number, code_line):
    for index in option_line_number:
        if index < len(func_line):
            traget_line = remove_special_chars(func_line[index])
            if traget_line in code_line:
                return index
    return False

def get_line_numbers(cwe_id, repo_name, func_name, git_url, code_lines, code_func, flag):
    # Initialize dictionary to store line numbers for each code line
    if not code_lines:
        return []
    line_numbers = []
    func_line = code_func.split('\n')
    # Find line numbers for each code line
    for code_line in code_lines:
        if len(code_line) == 1:
            continue
        option_line_number = []
        found = False
        for i, line in enumerate(func_line, 1):
            code_line, line = remove_special_chars(code_line), remove_special_chars(line)
            if line in code_line:
                option_line_number.append(i)
            if code_line == line: 
                line_numbers.append(i)
                found = True
                break
        if not found:
            if option_line_number:
                if_found_line = handle_line_break(func_line, option_line_number, code_line)
                if if_found_line:
                    line_numbers.append(if_found_line)
                    found = True
                    # raise Exception(f"{repo_name}/{func_name}_{flag} 未能找到代码行: {code_line}\ngit_url:{git_url}")
        if not found:
            logger.warning(f"{cwe_id}:{repo_name}/{func_name}_{flag} 未能找到代码行: {code_line}\ngit_url:{git_url}")  
    return line_numbers


def line_to_number(cwe_id, repo_name, func_name, git_url, added_line, deleted_line, func_before, func_after):
    added_line_numbers = get_line_numbers(cwe_id, repo_name, func_name, git_url, added_line, func_after, 'added')
    deleted_line_numbers = get_line_numbers(cwe_id, repo_name, func_name, git_url, deleted_line, func_before, 'deleted')
    if added_line_numbers == [] and deleted_line_numbers == []:
        logger.warning(f"{cwe_id}:{repo_name}/{func_name}:返回的是空集合\ngit_url:{git_url}")  
    return added_line_numbers, deleted_line_numbers
  

def main():
    return 

if __name__ == '__main__':
    main()