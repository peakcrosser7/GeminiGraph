import struct
import argparse

def convert_edge_list_to_binary(input_file: str, output_file: str, header_lines=1):
    """
    将边表文件转换为二进制文件
    
    参数:
    input_file: 输入的文本文件路径
    output_file: 输出的二进制文件路径
    header_lines: 需要跳过的文件头行数
    """
    try:
        # 打开输出二进制文件
        with open(output_file, 'wb') as fout:
            # 读取输入文件，跳过文件头
            with open(input_file, 'r') as fin:
                # 跳过文件头
                for _ in range(header_lines):
                    next(fin)
                
                # 处理每一行
                for line in fin:
                    # 分割每行，获取两个节点ID
                    try:
                        src, dst = map(int, line.strip().split())
                        # 写入二进制文件，每个ID用4字节（32位整数）表示
                        fout.write(struct.pack('II', src, dst))
                    except ValueError as e:
                        print(f"跳过无效行: {line.strip()}")
                        continue
                    
        print(f"转换完成！输出文件: {output_file}")
        
        # 显示一些基本信息
        file_size = os.path.getsize(output_file)
        edge_count = file_size // (4 * 2)  # 每条边占8字节
        print(f"边数量: {edge_count}")
        
    except Exception as e:
        print(f"发生错误: {str(e)}")

def main():
    # 设置命令行参数
    parser = argparse.ArgumentParser(description='将边表文件转换为二进制格式')
    parser.add_argument('input', help='输入文本文件路径')
    parser.add_argument('output', help='输出二进制文件路径')
    parser.add_argument('--header', type=int, default=1, help='需要跳过的文件头行数(默认为1)')
    
    args = parser.parse_args()
    
    # 执行转换
    convert_edge_list_to_binary(args.input, args.output, args.header)

if __name__ == "__main__":
    import os
    main()
