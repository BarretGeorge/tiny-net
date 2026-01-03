package main

import (
	"fmt"
	"strings"

	"github.com/emicklei/proto"
)

// APIInfo 用于存储提取出的接口信息
type APIInfo struct {
	ServiceName string
	MethodName  string
	HTTPMethod  string // GET, POST, etc.
	Path        string
	Comment     string
}

func main() {
	// 你的原始 proto 文件内容
	src := `syntax = "proto3";

service HelloService {
  // 获取问候信息
  rpc SayHello (HelloRequest) returns (HelloResponse) {
    option (google.api.http) = {
      post: "/v1/hello:sayHello"
      body: "*"
    };
  }

  // 获取问候信息列表
  rpc ListHellos (HelloRequest) returns (stream HelloResponse) {
    option (google.api.http) = {
      get: "/v1/hello:listHellos"
      body: "*"
    };
  }
}

message HelloRequest {
  string name = 1;
  int32 id = 2;
  string email = 3;
}

message HelloResponse{
  string message = 1;
}`

	reader := strings.NewReader(src)
	parser := proto.NewParser(reader)

	// 1. 解析 Proto 文件生成 AST 定义 (Definition)
	definition, err := parser.Parse()
	if err != nil {
		panic(err)
	}

	var apis []APIInfo

	// 2. 遍历 AST (Walk)
	proto.Walk(definition, func(v proto.Visitee) {
		// 我们只关心 Service 节点
		if s, ok := v.(*proto.Service); ok {
			// 遍历 Service 中的每一个元素 (RPC 等)
			for _, ele := range s.Elements {
				if rpc, ok := ele.(*proto.RPC); ok {
					info := APIInfo{
						ServiceName: s.Name,
						MethodName:  rpc.Name,
						Comment:     cleanComment(rpc.Comment), // 处理注释
					}

					// 3. 解析 RPC 中的 Option (主要处理 google.api.http)
					for _, opt := range rpc.Options {
						// 检查是否是 google.api.http 选项
						// 注意：AST 中名称可能包含括号，如 "(google.api.http)"
						if strings.Contains(opt.Name, "google.api.http") {
							parseHTTPOption(opt, &info)
						}
					}
					apis = append(apis, info)
				}
			}
		}
	})

	// 3. 输出结果
	fmt.Printf("%-20s | %-6s | %-30s | %s\n", "RPC Method", "HTTP", "Path", "Comment")
	fmt.Println(strings.Repeat("-", 80))
	for _, api := range apis {
		fmt.Printf("%-20s | %-6s | %-30s | %s\n",
			api.MethodName,
			api.HTTPMethod,
			api.Path,
			api.Comment)
	}
}

// cleanComment 清理注释中的 // 和换行符
func cleanComment(c *proto.Comment) string {
	if c == nil {
		return ""
	}
	var lines []string
	for _, line := range c.Lines {
		lines = append(lines, strings.TrimSpace(line))
	}
	return strings.Join(lines, " ")
}

// parseHTTPOption 解析 option (google.api.http) = { ... } 结构
func parseHTTPOption(opt *proto.Option, info *APIInfo) {
	// google.api.http 的值通常是一个聚合常量 (AggregatedConstants)
	// 结构类似于 map: { post: "...", body: "..." }
	for _, part := range opt.AggregatedConstants {
		// part.Name 是 key (例如 "post", "get", "body")
		// part.Literal.Source 是 value (例如 "/v1/..." 或 "*")

		key := strings.TrimSpace(part.Name)
		val := strings.Trim(part.Literal.Source, "\"") // 去掉引号

		switch key {
		case "get":
			info.HTTPMethod = "GET"
			info.Path = val
		case "post":
			info.HTTPMethod = "POST"
			info.Path = val
		case "put":
			info.HTTPMethod = "PUT"
			info.Path = val
		case "delete":
			info.HTTPMethod = "DELETE"
			info.Path = val
		case "patch":
			info.HTTPMethod = "PATCH"
			info.Path = val
		}
	}
}