import pygraphviz as pgv
from moose import *
import numpy as np


def xyPosition(objInfo,xory):
    try:
        return(float(element(objInfo).getField(xory)))
    except ValueError:
        return (float(0))

def setupMeshObj(modelRoot):
    ''' Setup compartment and its members pool,reaction,enz cplx under self.meshEntry dictionaries \ 
    self.meshEntry with "key" as compartment, 
    value is key2:list where key2 represents moose object type,list of objects of a perticular type
    e.g self.meshEntry[meshEnt] = { 'reaction': reaction_list,'enzyme':enzyme_list,'pool':poollist,'cplx': cplxlist }
    '''
    xmin = 0.0
    xmax = 1.0
    ymin = 0.0
    ymax = 1.0
    print "######",modelRoot,le('/'+modelRoot)
    meshEntry = {}
    xcord = []
    ycord = []
    meshEntryWildcard = modelRoot+'/##[TYPE=MeshEntry]'
    for meshEnt in wildcardFind(meshEntryWildcard):
        mollist = []
        realist = []
        enzlist = []
        cplxlist = []
        tablist = []
        for reItem in Neutral(meshEnt).getNeighbors('remeshReacs'):
            if isinstance(element(reItem),moose.EnzBase):
                enzlist.append(reItem)
            else:
                realist.append(reItem)
            objInfo = reItem.path+'/info'

            xcord.append(xyPosition(objInfo,'x'))
            ycord.append(xyPosition(objInfo,'y'))
            #print "###",reItem,xyPosition(objInfo,'x'),xyPosition(objInfo,'y')
        for mitem in Neutral(meshEnt).getNeighbors('remesh'):
            """ getNeighbors(remesh) has eliminating GSLStoich """
            if isinstance(element(mitem[0].parent),CplxEnzBase):
                cplxlist.append(mitem)
                objInfo = (mitem[0].parent).path+'/info'
            elif isinstance(element(mitem),moose.PoolBase):
                mollist.append(mitem)
                objInfo = mitem.path+'/info'
            xcord.append(xyPosition(objInfo,'x'))
            ycord.append(xyPosition(objInfo,'y'))
       
        compt = meshEnt.parent[0].path+'/##[TYPE=StimulusTable]'
        for table in wildcardFind(compt):
            tablist.append(table)
            objInfo = table.path+'/info'
            xcord.append(xyPosition(objInfo,'x'))
            ycord.append(xyPosition(objInfo,'y'))
       
        meshEntry[meshEnt] = {'enzyme':enzlist,
                              'reaction':realist,
                              'pool':mollist,
                              'cplx':cplxlist,
                              'table':tablist
                              }
        xmin = min(xcord)
        xmax = max(xcord)
        ymin = min(ycord)
        ymax = max(ycord)

        noPositionInfo = len(np.nonzero(xcord)[0]) == 0 \
            and len(np.nonzero(ycord)[0]) == 0
    return(meshEntry,xmin,xmax,ymin,ymax,noPositionInfo)

def setupItem(modlePath,cntDict):
    '''This function collects information of what is connected to what. \
    eg. substrate and product connectivity to reaction's and enzyme's \
    sumtotal connectivity to its pool are collected '''

    zombieType = ['ReacBase','EnzBase','FuncBase','StimulusTable']
    for baseObj in zombieType:
        path = modlePath+'/##[ISA='+baseObj+']'
        if ( (baseObj == 'ReacBase') or (baseObj == 'EnzBase')):
            for items in wildcardFind(path):
                sublist = []
                prdlist = []
                for sub in items[0].getNeighbors('sub'): 
                    sublist.append((sub,'s'))
                for prd in items[0].getNeighbors('prd'):
                    prdlist.append((prd,'p'))
                if (baseObj == 'CplxEnzBase') :
                    for enzpar in items[0].getNeighbors('toEnz'):
                        sublist.append((enzpar,'t'))
                    for cplx in items[0].getNeighbors('cplxDest'):
                        prdlist.append((cplx,'cplx'))
                if (baseObj == 'EnzBase'):
                    for enzpar in items[0].getNeighbors('enzDest'):
                        sublist.append((enzpar,'t'))
                cntDict[items] = sublist,prdlist
        elif baseObj == 'FuncBase':
            #ZombieSumFunc adding inputs
            for items in wildcardFind(path):
                inputlist = []
                outputlist = []
                funplist = []
                nfunplist = []
                for inpt in items[0].getNeighbors('input'):
                    inputlist.append((inpt,'st'))
                for funcbase in items[0].getNeighbors('output'): 
                    funplist.append(funcbase)
                if(len(funplist) > 1): print "SumFunPool has multiple Funpool"
                else:  cntDict[funplist[0]] = inputlist
        else:
            for tab in wildcardFind(path):
                tablist = []
                for tabconnect in tab[0].getNeighbors('output'):
                    tablist.append((tabconnect,'tab'))
                cntDict[tab] = tablist

def autoCoordinates(G,meshEntry,srcdesConnection):
    #for cmpt,memb in meshEntry.items():
    #    print memb
    for cmpt,memb in meshEntry.items():
        for enzObj in find_index(memb,'enzyme'):
            G.add_node(enzObj.path,label=element(enzObj).getField('name'),shape='ellipse',color='',style='filled',fontname='Helvetica',fontsize=12,fontcolor='blue')
    for cmpt,memb in meshEntry.items():
        for poolObj in find_index(memb,'pool'):
            G.add_node(poolObj.path,label=element(poolObj).getField('name'),shape='box',color='yellow',style='filled',fontname='Helvetica',fontsize=12,fontcolor='blue')
        for cplxObj in find_index(memb,'cplx'):
            G.add_node(cplxObj.path,label=element(cplxObj).getField('name'),shape='box',color='yellow',style='filled',fontname='Helvetica',fontsize=12,fontcolor='blue')
        for reaObj in find_index(memb,'reaction'):
            G.add_node(reaObj.path,label=element(reaObj).getField('name'),shape='ellipse',color='',style='filled',fontname='Helvetica',fontsize=12,fontcolor='blue')
        
    for inn,out in srcdesConnection.items():
        if (inn.class_ =='ZombieReac'): arrowcolor = 'green'
        elif(inn.class_=='ZombieEnz'): arrowcolor = 'red'
        else: arrowcolor = 'blue'
        #print "$",arrowcolor,inn
        if isinstance(out,tuple):
            if len(out[0])== 0:
                print "Reaction or Enzyme doesn't input mssg"
            else:
                for items in (items for items in out[0] ):
                    G.add_edge(items[0].path,inn[0].path,color=arrowcolor,spline="spline",arrowhead="none")
            if len(out[1]) == 0:
                print "Reaction or Enzyme doesn't output mssg"
            else:
                for items in (items for items in out[1] ):
                    ge = G.add_edge(inn[0].path,items[0].path,color=arrowcolor,spline="spline",arrowhead="normal")
        elif isinstance(out,list):
            if len(out) == 0:
                print "Func pool doesn't have sumtotal"
            else:
                for items in (items for items in out ):
                    G.add_edge(items[0].path,inn[0].path,color=arrowcolor,spline="spline",arrowhead="normal")
    G.layout(prog='dot')
    #G.draw('/home/harsha/Trash/pygraphviz/77'+'.png',prog='dot',format='png')

def find_index(value, key):
    """ Value.get(key) to avoid expection which would raise if empty value in dictionary for a given key """
    if value.get(key) != None:
        return value.get(key)
    else:
        raise ValueError('no dict with the key found')