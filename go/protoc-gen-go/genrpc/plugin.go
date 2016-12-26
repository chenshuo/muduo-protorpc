package genrpc

import (
	"fmt"
	"os"
	"strconv"

	descriptor "code.google.com/p/goprotobuf/protoc-gen-go/descriptor"
	"code.google.com/p/goprotobuf/protoc-gen-go/generator"
)

type Plugin struct {
	g *generator.Generator
}

func (p *Plugin) Name() string {
	_ = fmt.Printf
	_ = os.Stderr
	return "Muduo ProtoRPC"
}

func (p *Plugin) Init(g *generator.Generator) {
	p.g = g
}

func (p *Plugin) Generate(file *generator.FileDescriptor) {
	if file.Service == nil {
		return
	}
	generateServices(p.g, file)
}

func (p *Plugin) GenerateImports(file *generator.FileDescriptor) {
	if file.Service == nil {
		return
	}
	p.g.P("// RPC Imports")
	// FIXME: RegisterUniquePackageName
	p.g.P(`import "io"`)
	p.g.P(`import "net/rpc"`)
	p.g.P(`import "github.com/chenshuo/muduo-protorpc/go/muduorpc"`)
}

func generateServices(g *generator.Generator, file *generator.FileDescriptor) {
	g.P("// Services")
	g.P()
	for i, s := range file.Service {
		if i > 0 {
			g.P("/////////////////////////////////")
		}
		generateService(g, file.Package, s)
	}
}

func generateService(g *generator.Generator, pkg *string, s *descriptor.ServiceDescriptorProto) {

	full_name := *s.Name
	if pkg != nil {
		full_name = *pkg + "." + *s.Name
	}

	// interface
	g.P()
	g.P("type ", s.Name, " interface {")
	g.In()
	for _, m := range s.Method {
		g.P(generator.CamelCase(*m.Name),
			"(req *", typeName(g, *m.InputType),
			", resp *", typeName(g, *m.OutputType), ") error")
	}
	g.Out()
	g.P("}")

	// register
	g.P()
	g.P("func Register", s.Name, "(service ", s.Name, ")  {")
	g.In()
	g.P("rpc.RegisterName(", strconv.Quote(full_name), ", service)")
	g.Out()
	g.P("}")

	client_name := *s.Name + "Client"
	// new client
	g.P()
	g.P("func New", client_name, "(conn io.ReadWriteCloser) *", client_name, " {")
	g.In()
	g.P("codec := muduorpc.NewClientCodec(conn)")
	g.P("client := rpc.NewClientWithCodec(codec)")
	g.P("return &", client_name, "{client}")
	g.Out()
	g.P("}")

	// client
	g.P()
	g.P("type ", client_name, " struct {")
	g.In()
	g.P("client *rpc.Client")
	g.Out()
	g.P("}")

	// client methods
	g.P()
	g.P("func (c *", client_name, ") Close () error {")
	g.In()
	g.P("return c.client.Close()")
	g.Out()
	g.P("}")

	for _, m := range s.Method {
		g.P()
		g.P("func (c *", client_name, ") ", generator.CamelCase(*m.Name),
			"(req *", typeName(g, *m.InputType),
			", resp *", typeName(g, *m.OutputType), ") error {")
		g.In()
		name := full_name + "." + *m.Name
		g.P("return c.client.Call(", strconv.Quote(name), ", req, resp)")
		g.Out()
		g.P("}")
	}
}

func typeName(g *generator.Generator, t string) string {
	return g.TypeName(g.ObjectNamed(t))
}

func init() {
	generator.RegisterPlugin(new(Plugin))
}
