#!/usr/bin/env python3
import argparse
import xml.dom.minidom

def in_args(args):
    return filter(lambda arg: arg.getAttribute("type") != "new_id", args)

def out_args(args):
    return filter(lambda arg: arg.getAttribute("type") == "new_id", args)

def iface_c_type(iface_name):
    return "::" + iface_name

def iface_cpp_type(iface_name):
    return iface_name

def arg_c_intype(elem):
    type_attr = elem.getAttribute("type")
    if type_attr == "int":
        return "std::int32_t"
    if type_attr == "uint":
        return "std::uint32_t"
    if type_attr == "string":
        return "const char*"
    if type_attr == "array":
        return "wl_array*"
    if type_attr == "object":
        return iface_c_type(elem.getAttribute("interface")) + "*"
    return f"(UNKNOWN:{type_attr})"

def arg_c_decl(arg):
    return arg_c_intype(arg) + " " + arg.getAttribute("name")

def event_arglist_c(iface_name, event):
    return ", ".join(["void* data", f"{iface_c_type(iface_name)}*"] + list(map(arg_c_decl, event.getElementsByTagName("arg"))))

def arg_cpp_type(arg):
    type_attr = arg.getAttribute("type")
    if type_attr == "int":
        return "std::int32_t"
    if type_attr == "uint":
        return "std::uint32_t"
    if type_attr == "string":
        return "std::string"
    if type_attr == "array":
        return "std::vector<std::byte>"
    if type_attr == "object":
        return arg.getAttribute("interface") + "&"
    if type_attr == "new_id":
        return arg.getAttribute("interface")
    if enum_attr:
        return arg.getAttribute("enum")
    return f"(UNKNOWN:{type_attr})"

def arg_cpp_decl(arg):
    return arg_cpp_type(arg) + " " + arg.getAttribute("name")

def req_cpp_return_type(all_args):
    outs = list(out_args(all_args))
    if len(outs) == 0:
        return "void"
    elif len(outs) == 1:
        return arg_cpp_type(all_args[0])
    else:
        return "std::tuple<" + ", ".join(map(arg_cpp_type, outs)) + ">"

def req_arglist_cpp(all_args):
    return ", ".join(map(arg_cpp_decl, in_args(all_args)))
    
def req_cpp_decl(req):
    if request.getAttribute("type") != "destructor":
        name = request.getAttribute("name")
        all_args = request.getElementsByTagName("arg")
        return req_cpp_return_type(all_args) + " " + name + "(" + req_arglist_cpp(all_args) + ");"
    else:
        return "void dtor();"

def handler_ident(event):
    return "handle_" + event.getAttribute("name")
    
arg_parser = argparse.ArgumentParser(description="Generate .hpp from wayland protocol specifications")
arg_parser.add_argument("what", choices=["client-header", "private-code"])
arg_parser.add_argument("input", metavar="infile", type=argparse.FileType('r'), help="path to xml wayland protocol to generate header from")
arg_parser.add_argument("output", metavar="outfile", type=argparse.FileType('w'), nargs="?", default="-", help="destination path of generated c++ header (default: print to stdout)")
args = arg_parser.parse_args()

dom = xml.dom.minidom.parse(args.input)
o = lambda s: args.output.write(s)
protocol = dom.documentElement.getAttribute("name")
namespace = "wl" if protocol == "wayland" else "wlp"

if args.what == "client-header":
    o(f"#ifndef SI_{namespace.upper()}_HPP_INCLUDED\n")
    o(f"#define SI_{namespace.upper()}_HPP_INCLUDED\n")
    o( "\n")
    o( "#include <wayland-client.h>\n")
    o(f"#include <wayland-client-xdg-shell.h>\n")
    o( "#include <memory>\n")
    o( "#include <cstdint>\n")
    o( "#include <vector>\n")
    o( "#include <boost/signals2/signal.hpp>\n\n")
    o(f"namespace si::{namespace} ")
    o( "{\n")
    for iface in dom.documentElement.getElementsByTagName("interface"):
        iface_name = iface.getAttribute("name")
        o(f"    struct {iface_name};" + "\n")
    o( "\n")
    for iface in dom.documentElement.getElementsByTagName("interface"):
        iface_name = iface.getAttribute("name")
        o(f"    struct {iface_name}_deleter " + "{\n")
        o(f"        void operator()({iface_c_type(iface_name)}*) const;\n")
        o( "    };\n")
        o(f"    class {iface_name} " + "{\n")
        o(f"        std::unique_ptr<{iface_c_type(iface_name)}, {iface_name}_deleter> const hnd;\n")
        for event in iface.getElementsByTagName("event"):
            o(f"        static void {handler_ident(event)}({event_arglist_c(iface_name, event)});" + "\n")
        if iface.getElementsByTagName("event"):
            o(f"        ::{iface_name}_listener event_listener = " + "{ " + ", ".join(map(handler_ident, iface.getElementsByTagName("event"))) + " };\n")
        o( "    public:\n")
        o(f"        explicit {iface_name}({iface_c_type(iface_name)}*);\n")
        for event in iface.getElementsByTagName("event"):
            all_args = event.getElementsByTagName("arg")
            o(f"        boost::signals2::signal<void(" + ", ".join(map(arg_cpp_type, event.getElementsByTagName("arg"))) + ")> on_" + event.getAttribute("name") + ";\n")
        for request in iface.getElementsByTagName("request"):
            if request.getAttribute("type") != "destructor":
                o("        " + req_cpp_decl(request) + "\n")
        o( "    };\n")
    o( "}\n\n")
    o( "#endif\n\n\n")
elif args.what == "private-code":
    for iface in dom.documentElement.getElementsByTagName("interface"):
        iface_name = iface.getAttribute("name")
        o(f"void si::{namespace}::{iface_name}_deleter::operator()({iface_c_type(iface_name)}* ptr) const " + "{\n")
        o(f"    ::{iface_name}_destroy(ptr);" + "\n")
        o( "}\n")
        for event in iface.getElementsByTagName("event"):
            all_args = event.getElementsByTagName("arg")
            o(f"void si::{namespace}::{iface_name}::{handler_ident(event)}({event_arglist_c(iface_name, event)})" + " {\n")
            o(f"    {iface_name}& wrapper = *reinterpret_cast<{iface_name}*>(data);" + "\n")
            o(f"    wrapper.on_{event.getAttribute('name')}(" + ", ".join(map(lambda arg: arg.getAttribute("name"), all_args)) + ");\n")
            o( "}\n")
        o(f"si::{namespace}::{iface_name}::{iface_name}({iface_c_type(iface_name)}* ptr): hnd(ptr)" + " {\n")
        if iface.getElementsByTagName("event"):
            o(f"    ::{iface_name}_add_listener(hnd.get(), &event_listener, this)" + ";\n")
        o( "}\n")
        for request in filter(lambda req: req.getAttribute("type") != "destructor" , iface.getElementsByTagName("request")):
            req_name = request.getAttribute("name")
            all_args = request.getElementsByTagName("arg")
            o(f"{req_cpp_return_type(all_args)} si::{namespace}::{iface_name}::{req_name}({req_arglist_cpp(all_args)})" + " {\n")
            o(f"    return {req_cpp_return_type(all_args)}(::{iface_name}_{req_name}(" + ", ".join(["hnd.get()"] + list(map(lambda arg: arg.getAttribute("name"), in_args(all_args)))) + "));\n")
            o( "}\n")
