# vim: fileencoding=utf8
""" Lua domain based on sphinx JavaScript domain
"""
import re
from sphinx import addnodes
from sphinx.domains import Domain, ObjType
from sphinx.locale import l_, _
from sphinx.directives import ObjectDescription
from sphinx.roles import XRefRole
from sphinx.domains.python import _pseudo_parse_arglist
from sphinx.util.nodes import make_refnode
from sphinx.util.docfields import Field, GroupedField, TypedField

separators="([\.:])"

class LuaObject(ObjectDescription):
  has_arguments = False
  display_prefix = None

  def handle_signature(self, sig, signode):
    sig = sig.strip()
    if '(' in sig and sig[-1:] == ')':
      prefix, arglist = sig.split('(', 1)
      prefix = prefix.strip()
      arglist = arglist[:-1].strip()
    else:
      prefix = sig
      arglist = None
    if re.search(separators, prefix):
      separator = re.findall(separators, prefix)[-1]
      nameprefix, name = prefix.rsplit(separator, 1)
    else:
      separator = None
      nameprefix = None
      name = prefix

    objectname = self.env.temp_data.get('lua:object')
    if nameprefix:
      if objectname:
        nameprefix = objectname + '.' + nameprefix
      fullname = nameprefix + separator + name
    elif objectname:
      fullname = objectname + '.' + name
    else:
      objectname = ''
      fullname = name

    signode['object'] = objectname
    signode['fullname'] = fullname

    if self.display_prefix:
      signode += addnodes.desc_annotation(self.display_prefix,
                        self.display_prefix)
    if nameprefix:
      signode += addnodes.desc_addname(nameprefix + separator, nameprefix + separator)
    signode += addnodes.desc_name(name, name)
    if self.has_arguments:
      if not arglist:
        signode += addnodes.desc_parameterlist()
      else:
        _pseudo_parse_arglist(signode, arglist)
    return fullname, nameprefix

  def add_target_and_index(self, name_obj, sig, signode):
    objectname = self.options.get(
      'object', self.env.temp_data.get('lua:object'))
    fullname = name_obj[0]
    if fullname not in self.state.document.ids:
      signode['names'].append(fullname)
      signode['ids'].append(fullname.replace('$', '_S_'))
      signode['first'] = not self.names
      self.state.document.note_explicit_target(signode)
      objects = self.env.domaindata['lua']['objects']
      if fullname in objects:
        self.state_machine.reporter.warning(
          'duplicate object description of %s, ' % fullname +
          'other instance in ' +
          self.env.doc2path(objects[fullname][0]),
          line=self.lineno)
      objects[fullname] = self.env.docname, self.objtype

    indextext = self.get_index_text(objectname, name_obj)
    if indextext:
      self.indexnode['entries'].append(('single', indextext,
                        fullname.replace('$', '_S_'),
                        ''))

  def get_index_text(self, objectname, name_obj):
    name, obj = name_obj
    if self.objtype == 'function':
      if not obj:
        return _('%s() (function)') % name
      return _('%s() (%s method)') % (name, obj)
    elif self.objtype == 'class':
      return _('%s() (class)') % name
    elif self.objtype == 'data':
      return _('%s (global variable or constant)') % name
    elif self.objtype == 'attribute':
      return _('%s (%s attribute)') % (name, obj)
    return ''


class LuaCallable(LuaObject):
  has_arguments = True
  doc_field_types = [
    TypedField('arguments', label=l_('Arguments'),
           names=('argument', 'arg', 'parameter', 'param'),
           typerolename='func', typenames=('paramtype', 'type')),
    Field('returnvalue', label=l_('Returns'), has_arg=False,
        names=('returns', 'return')),
    Field('returntype', label=l_('Return type'), has_arg=False,
        names=('rtype',)),
  ]


class LuaConstructor(LuaCallable):
  display_prefix = 'class '

class LuaXRefRole(XRefRole):
  def process_link(self, env, refnode, has_explicit_title, title, target):
    refnode['lua:object'] = env.temp_data.get('lua:object')
    if not has_explicit_title:
      title = title.lstrip('.')
      target = target.lstrip('~')
      if title[0:1] == '~':
        title = title[1:]
        dot = title.rfind('.')
        if dot != -1:
          title = title[dot+1:]
    if target[0:1] == '.':
      target = target[1:]
      refnode['refspecific'] = True
    return title, target


class LuaDomain(Domain):
  """Lua language domain."""
  name = 'lua'
  label = 'Lua'
  object_types = {
    'function':  ObjType(l_('function'),  'func'),
    'class':   ObjType(l_('class'),   'class'),
    'data':    ObjType(l_('data'),    'data'),
    'attribute': ObjType(l_('attribute'), 'attr'),
  }
  directives = {
    'function':  LuaCallable,
    'class':   LuaConstructor,
    'data':    LuaObject,
    'attribute': LuaObject,
  }
  roles = {
    'func':  LuaXRefRole(fix_parens=True),
    'class': LuaXRefRole(fix_parens=True),
    'data':  LuaXRefRole(),
    'attr':  LuaXRefRole(),
  }
  initial_data = {
    'objects': {}, 
  }

  def clear_doc(self, docname):
    for fullname, (fn, _) in self.data['objects'].items():
      if fn == docname:
        del self.data['objects'][fullname]

  def find_obj(self, env, obj, name, typ, searchorder=0):
    if name[-2:] == '()':
      name = name[:-2]
    objects = self.data['objects']
    newname = None
    if searchorder == 1:
      if obj and obj + '.' + name in objects:
        newname = obj + '.' + name
      elif obj and obj + ':' + name in objects:
        newname = obj + ':' + name
      else:
        newname = name
    else:
      if name in objects:
        newname = name
      elif obj and obj + '.' + name in objects:
        newname = obj + '.' + name
      elif obj and obj + ':' + name in objects:
        newname = obj + ':' + name
    return newname, objects.get(newname)

  def resolve_xref(self, env, fromdocname, builder, typ, target, node,
           contnode):
    objectname = node.get('lua:object')
    searchorder = node.hasattr('refspecific') and 1 or 0
    name, obj = self.find_obj(env, objectname, target, typ, searchorder)
    if not obj:
      return None
    return make_refnode(builder, fromdocname, obj[0],
              name.replace('$', '_S_'), contnode, name)

  def get_objects(self):
    for refname, (docname, type) in self.data['objects'].iteritems():
      yield refname, refname, type, docname, \
          refname.replace('$', '_S_'), 1

def setup(app):
  app.add_domain(LuaDomain)
