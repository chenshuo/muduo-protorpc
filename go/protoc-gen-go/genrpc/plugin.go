package genrpc

import (
	"fmt"
	"os"

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
	g := p.g
	g.P("// Services")
	p.generateInterfaces(file)
	generateStubs(g, file)
}

func (p *Plugin) GenerateImports(file *generator.FileDescriptor) {
	if file.Service == nil {
		return
	}
	p.g.P("// Generate Imports")
}

func (p *Plugin) generateInterfaces(file *generator.FileDescriptor) {
	g := p.g
	for _, s := range file.Service {
		g.P()
		g.P("type ", s.Name, " interface {")
		g.In()
		for _, m := range s.Method {
			g.P(generator.CamelCase(*m.Name),
				"(req *", p.typeName(*m.InputType),
				", resp *", p.typeName(*m.OutputType), ") error")
		}
		g.Out()
		g.P("}")
	}
}

func (p *Plugin) typeName(t string) string {
	return p.g.TypeName(p.g.ObjectNamed(t))
}

func generateStubs(g *generator.Generator, file *generator.FileDescriptor) {
	// TODO
}

func init() {
	generator.RegisterPlugin(new(Plugin))
}
