import os
import subprocess

from collections import OrderedDict


# The DOT Language
# See http://www.graphviz.org/doc/info/lang.html


class AttrList(dict):
    def __str__(self):
        so = ' '.join(['{}="{}"'.format(k, str(v).replace('\"', '\\\"')) for k,v in self.items()])
        if len(so) > 0:
            return '[%s]' % so
        else:
            return ''


class Graph:

    class Node:
        def __init__(self, name, **kwargs):
            self.name = name
            self.attrs = AttrList(kwargs)

        def __str__(self):
            return ' '.join((str(self.name), str(self.attrs)))


    class Edge:
        def __init__(self, directed, node_from, node_to, **kwargs):
            self.directed = directed
            self.node_from = node_from
            self.node_to = node_to
            self.attrs = AttrList(kwargs)

        def __str__(self):
            return ' '.join((
                str(self.node_from),
                '->' if self.directed else '--',
                str(self.node_to),
                str(self.attrs) ))


    def __init__(self, directed=False):
        self.directed = directed
        self.nodes = OrderedDict()
        self.edges = OrderedDict()
        self.graph_attr = AttrList()
        self.node_attr = AttrList()
        self.edge_attr = AttrList()

    def node(self, name, **kwargs):
        node = self.Node(name, **kwargs)
        self.nodes[node.name] = node

    def edge(self, node_from, node_to, **kwargs):
        edge = self.Edge(self.directed, node_from, node_to, **kwargs)
        self.edges[(edge.node_from, edge.node_to)] = edge

    def update_edge(self, node_from, node_to=None, **kwargs):
        if node_to is not None:
            node_from = (node_from, node_to)

        edge = self.edges[node_from]
        edge.attrs.update(kwargs)

    def get_edge_attrs(self, node_from, node_to=None):
        if node_to is not None:
            node_from = (node_from, node_to)

        edge = self.edges[node_from]
        return edge.attrs

    def render(self, stream=None):
        doc = [
            self._preamble(),
            self._graph_attrs(),
            self._node_attrs(),
            self._edge_attrs(),
            self._nodes(),
            self._edges(),
            self._postamble(),
        ]
        doc = filter(None, doc)

        so = '\n'.join(doc) + '\n'

        if stream is None:
            return so

        so = bytes(so, 'utf-8')
        stream.write(so)

    def _preamble(self):
        return 'digraph {' if self.directed else 'graph {'

    def _postamble(self):
        return '}'

    def _graph_attrs(self):
        return self._attrs('graph', self.graph_attr)

    def _node_attrs(self):
        return self._attrs('node', self.node_attr)

    def _edge_attrs(self):
        return self._attrs('edge', self.edge_attr)

    def _attrs(self, name, attrs):
        if len(attrs) > 0:
            return '    {} {}'.format(name, attrs)

    def _nodes(self):
        return '\n'.join(['    %s' % node for node in self.nodes.values()])

    def _edges(self):
        return '\n'.join(['    %s' % edge for edge in self.edges.values()])


class Plotter:
    def __init__(self, engine='neato', **kwargs):
        self.engine = engine
        self.kwargs = kwargs

    def plot(self, graph, target=None, format=None):
        if hasattr(target, 'write'):
            self._write_file(target, graph, format=format)
        else:
            dirs = os.path.dirname(target)
            os.makedirs(dirs, exist_ok=True)

            with open(target, 'w') as fd:
                self._write_file(fd, graph, format=format)

    def _write_file(self, fd, graph, format=None):
        dot = graph.render()
        dot = bytes(dot, 'utf-8')

        kwargs = self.kwargs.copy()
        kwargs['-T'] = format

        cmd = self._command('dot', '-K', self.engine, **kwargs)

        subprocess.run(cmd, input=dot, stdout=fd)

    def _command(self, *args, **kwargs):
        cmd = list(filter(None, args))
        cmd.extend(x for k,v in kwargs.items() if v is not None for x in (k,v))
        return cmd
