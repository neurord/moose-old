# moose.py --- 
# 
# Filename: moose.py
# Description: 
# Author: Subhasis Ray
# Maintainer: 
# Copyright (C) 2010 Subhasis Ray, all rights reserved.
# Created: Sat Mar 12 14:02:40 2011 (+0530)
# Version: 
# Last-Updated: Thu Oct  4 20:04:06 2012 (+0530)
#           By: subha
#     Update #: 2170
# URL: 
# Keywords: 
# Compatibility: 
# 
# 

# Commentary: 
# 
# This is the primary moose module. It wraps _moose.so and adds some
# utility functions.
# 

# Change log:
# 
# Mon Sep 9 22:56:35 IST 2013 - Renamed some function parameters and
# updated docstrings to consistently follow numpy docstring
# convention.
# 

# Code:

from __future__ import print_function
import io
import warnings
import platform
import pydoc
from collections import defaultdict
from . import _moose
from ._moose import *
import __main__ as main

sequence_types = [ 'vector<double>',
                   'vector<int>',
                   'vector<long>',
                   'vector<unsigned int>',
                   'vector<float>',
                   'vector<unsigned long>',
                   'vector<short>',
                   'vector<Id>',
                   'vector<ObjId>' ]
known_types = ['void',
               'char',
               'short',
               'int',
               'unsigned int',
               'double',
               'float',
               'long',
               'unsigned long',
               'string',
               'vec',
               'melement'] + sequence_types

################################################################
# Wrappers for global functions
################################################################ 
    
def pwe():
    """Print present working element. Convenience function for GENESIS
    users. If you want to retrieve the element in stead of printing
    the path, use moose.getCwe()

    """
    pwe_ = _moose.getCwe()
    print(pwe_.getPath())
    return pwe_
    
def le(el=None):
    """List elements under `el` or current element if no argument
    specified.
    
    Parameters
    ----------
    el : str/melement/vec/None
        The element or the path under which to look. If `None`, children
         of current working element are displayed.

    Returns
    -------
    None

    """
    if el is None:
        el = getCwe()
    elif isinstance(el, str):
        if not exists(el):
            raise ValueError('no such element')
        el = element(el)
    elif isinstance(el, vec):
        el = el[0]    
    print('Elements under', el.path)
    for ch in el.children:
        print(ch.path)

ce = setCwe # ce is a GENESIS shorthand for change element.

def syncDataHandler(target):
    """Synchronize data handlers for target.

    Parameters
    ----------
    target : melement/vec/str
        Target element or vec or path string.

    Raises
    ------
    NotImplementedError
        The call to the underlying C++ function does not work.

    Notes
    -----
    This function is defined for completeness, but currently it does not work.

    """
    raise NotImplementedError('The implementation is not working for IntFire - goes to invalid objects. \
First fix that issue with SynBase or something in that line.')
    if isinstance(target, str):
        if not _moose.exists(target):
            raise ValueError('%s: element does not exist.' % (target))
        target = vec(target)
        _moose.syncDataHandler(target)

def showfield(el, field='*', showtype=False):
    """Show the fields of the element `el`, their data types and
    values in human readable format. Convenience function for GENESIS
    users.

    Parameters
    ----------
    el : melement/str
        Element or path of an existing element.

    field : str
        Field to be displayed. If '*' (default), all fields are displayed.

    showtype : bool
        If True show the data type of each field. False by default.

    Returns
    -------
    None

    """
    if isinstance(el, str):
        if not exists(el):
            raise ValueError('no such element')
        el = element(el)
    if field == '*':        
        value_field_dict = getFieldDict(el.className, 'valueFinfo')
        max_type_len = max(len(dtype) for dtype in value_field_dict.values())
        max_field_len = max(len(dtype) for dtype in value_field_dict)
        print('\n[', el.path, ']')
        for key, dtype in value_field_dict.items():
            if dtype == 'bad' or key == 'this' or key == 'dummy' or key == 'me' or dtype.startswith('vector') or 'ObjId' in dtype:
                continue
            value = el.getField(key)
            if showtype:
                typestr = dtype.ljust(max_type_len + 4)
                # The following hack is for handling both Python 2 and
                # 3. Directly putting the print command in the if/else
                # clause causes syntax error in both systems.
                print(typestr, end=' ')
            print(key.ljust(max_field_len + 4), '=', value)
    else:
        try:
            print(field, '=', el.getField(field))
        except AttributeError:
            pass # Genesis silently ignores non existent fields

def showfields(el, showtype=False):
    """Convenience function. Should be deprecated if nobody uses it.

    """
    warnings.warn('Deprecated. Use showfield(element, field="*", showtype=True) instead.', DeprecationWarning)
    showfield(el, field='*', showtype=showtype)

# Predefined field types and their human readable names    
finfotypes = [('valueFinfo', 'value field') , 
              ('srcFinfo', 'source message field'),
              ('destFinfo', 'destination message field'),
              ('sharedFinfo', 'shared message field'),
              ('lookupFinfo', 'lookup field')]

def listmsg(el):
    """Return a list containing the incoming and outgoing messages of
    `el`.

    Parameters
    ----------
    el : melement/vec/str
        MOOSE object or path of the object to look into.

    Returns
    -------
    msg : list
        List of Msg objects corresponding to incoming and outgoing 
        connections of `el`.

    """
    obj = element(el)
    ret = []
    for msg in obj.inMsg:
        ret.append(msg)
    for msg in obj.outMsg:
        ret.append(msg)
    return ret

def showmsg(el):
    """Print the incoming and outgoing messages of `el`.

    Parameters
    ----------
    el : melement/vec/str
        Object whose messages are to be displayed.

    Returns
    -------
    None

    """
    obj = element(el)
    print('INCOMING:')
    for msg in obj.msgIn:
        print(msg.e2.path, msg.destFieldsOnE2, '<---', msg.e1.path, msg.srcFieldsOnE1)
    print('OUTGOING:')
    for msg in obj.msgOut:
        print(msg.e1.path, msg.srcFieldsOnE1, '--->', msg.e2.path, msg.destFieldsOnE2)

def getfielddoc(tokens, indent=''):
    """Return the documentation for field specified by `tokens`.
    
    Parameters
    ----------
    tokens : (className, fieldName) str
        A sequence whose first element is a MOOSE class name and second 
        is the field name.
              
    indent : str
        indentation (default: empty string) prepended to builtin 
        documentation string.

    Returns
    -------
    docstring : str
        string of the form 
        `{indent}{className}.{fieldName}: {datatype} - {finfoType}\n{Description}\n`

    Raises
    ------
    NameError 
        If the specified fieldName is not present in the specified class.
    """
    assert(len(tokens) > 1)
    for ftype, rtype in finfotypes:
        cel = _moose.element('/classes/'+tokens[0])
        numfinfo = getField(cel, 'num_'+ftype, 'unsigned')
        finfo = element('/classes/%s/%s' % (tokens[0], ftype))
        for ii in range(numfinfo):
            oid = melement(finfo.getId(), 0, ii, 0)
            if oid.name == tokens[1]:
                return u'%s%s.%s: %s - %s\n\t%s\n' % \
                    (indent, tokens[0], tokens[1], 
                     oid.type, rtype, oid.docs)    
    raise NameError('`%s` has no field called `%s`' 
                    % (tokens[0], tokens[1]))
                    
    
def getmoosedoc(tokens):
    """Return MOOSE builtin documentation.
  
    Parameters
    ----------
    tokens : (className, [fieldName])
        tuple containing one or two strings specifying class name
        and field name (optional) to get documentation for.

    Returns
    -------
    docstring : str        
        Documentation string for class `className`.`fieldName` if both
        are specified, for the class `className` if fieldName is not
        specified. In the latter case, the fields and their data types
        and finfo types are listed.

    Raises
    ------
    NameError
        If class or field does not exist.
    
    """
    indent = '    '
    docstring = io.StringIO()
    if not tokens:
        return ""
    class_path = '/classes/%s' % (tokens[0])
    if exists(class_path):
        if len(tokens) == 1:
            docstring.write(u'%s\n' % (Cinfo(class_path).docs))
    else:
        raise NameError('name \'%s\' not defined.' % (tokens[0]))
    class_id = vec('/classes/%s' % (tokens[0]))
    if len(tokens) > 1:
        docstring.write(getfielddoc(tokens))
    else:
        for ftype, rname in finfotypes:
            docstring.write(u'\n*%s*\n' % (rname.capitalize()))
            numfinfo = getField(class_id[0], 'num_'+ftype, 'unsigned')
            finfo = vec('/classes/%s/%s' % (tokens[0], ftype))
            for ii in range(numfinfo):
                oid = melement(finfo, 0, ii, 0)
                docstring.write(u'%s%s: %s\n' %
                                (indent, oid.name, oid.type))
    ret = docstring.getvalue()
    docstring.close()
    return ret

# the global pager is set from pydoc even if the user asks for paged
# help once. this is to strike a balance between GENESIS user's
# expectation of control returning to command line after printing the
# help and python user's expectation of seeing the help via more/less.
pager=None

def doc(arg, paged=False):
    """Display the documentation for class or field in a class.
    
    Parameters
    ----------
    arg : str/class/melement/vec
        A a string specifying a moose class name and a field name
        separated by a dot. e.g., 'Neutral.name'. Prepending `moose.`
        is allowed. Thus moose.doc('moose.Neutral.name') is equivalent
        to the above.    
        It can also be string specifying just a moose class name or a
        moose class or a moose object (instance of melement or vec
        or there subclasses). In that case, the builtin documentation
        for the corresponding moose class is displayed.

    paged: bool    
        Whether to display the docs via builtin pager or print and
        exit. If not specified, it defaults to False and
        moose.doc(xyz) will print help on xyz and return control to
        command line.

    Returns
    -------
    None

    Raises
    ------
    NameError
        If class or field does not exist.

    """
    # There is no way to dynamically access the MOOSE docs using
    # pydoc. (using properties requires copying all the docs strings
    # from MOOSE increasing the loading time by ~3x). Hence we provide a
    # separate function.
    global pager
    if paged and pager is None:
        pager = pydoc.pager
    tokens = []
    text = ''
    if isinstance(arg, str):
        tokens = arg.split('.')
        if tokens[0] == 'moose':
            tokens = tokens[1:]
    elif isinstance(arg, type):
        tokens = [arg.__name__]
    elif isinstance(arg, melement) or isinstance(arg, vec):
        text = '%s: %s\n\n' % (arg.path, arg.className)
        tokens = [arg.className]
    if tokens:
        text += getmoosedoc(tokens)
    else:
        text += pydoc.gethelp(arg)
    if pager:
        pager(text)
    else:
        print(text)
                

# 
# moose.py ends here
