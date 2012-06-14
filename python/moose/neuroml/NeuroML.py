## Description: class NeuroML for loading NeuroML from single file into MOOSE
## Version 1.0 by Aditya Gilra, NCBS, Bangalore, India, 2011 for serial MOOSE
## Version 1.5 by Niraj Dudani, NCBS, Bangalore, India, 2012, ported to parallel MOOSE
## Version 1.6 by Aditya Gilra, NCBS, Bangalore, India, 2012, further changes for parallel MOOSE

"""
NeuroML.py is the preferred interface to read NeuroML files.

Instantiate NeuroML class, and thence use method:
readNeuroMLFromFile(...) to load NeuroML from a file:
(a) the file could contain all required levels 1, 2 and 3 - Network, Morph and Channel;
OR
(b) the file could have only L3 (network) with L1 (channels/synapses) and L2 (cells) spread over multiple files;
these multiple files should be in the same directory
named as <chan/syn_name>.xml or <cell_name>.xml or <cell_name>.morph.xml
(essentially as generated by neuroConstruct's export).

But, if your lower level L1 and L2 xml files are elsewise,
use the separate Channel, Morph and NetworkML loaders in moose.neuroml.<...> .

For testing, you can also call this from the command line with a neuroML file as argument.
"""

import moose
from moose.utils import *
from xml.etree import ElementTree as ET
from ChannelML import ChannelML
from MorphML import MorphML
from NetworkML import NetworkML
import string
from moose.neuroml.utils import *
import sys
from os import path

class NeuroML():

    def __init__(self):
        pass

    def readNeuroMLFromFile(self,filename,params={}):
        """
        For the format of params required to tweak what cells are loaded,
         refer to the doc string of NetworkML.readNetworkMLFromFile().
        Returns (populationDict,projectionDict),
         see doc string of NetworkML.readNetworkML() for details.
        """
        print "Loading neuroml file ... ", filename
        moose.Neutral('/library') # creates /library in MOOSE tree; elif present, wraps
        tree = ET.parse(filename)
        root_element = tree.getroot()
        self.model_dir = path.dirname( filename )
        self.lengthUnits = root_element.attrib['lengthUnits']
        self.temperature = CELSIUS_default # gets replaced below if tag for temperature is present
        self.temperature_default = True
        for meta_property in root_element.findall('.//{'+meta_ns+'}property'):
            tagname = meta_property.attrib['tag']
            if 'temperature' in tagname:
                self.temperature = float(meta_property.attrib['value'])
                self.temperature_default = False
        if self.temperature_default:
            print "Using default temperature of", self.temperature,"degrees Celsius."
        self.nml_params = {
                'temperature':self.temperature,
                'model_dir':self.model_dir,
        }

        #print "Loading channels and synapses into MOOSE /library ..."
        cmlR = ChannelML(self.nml_params)
        for channels in root_element.findall('.//{'+neuroml_ns+'}channels'):
            self.channelUnits = channels.attrib['units']
            for channel in channels.findall('.//{'+cml_ns+'}channel_type'):
                ## ideally I should read in extra params
                ## from within the channel_type element and put those in also.
                ## Global params should override local ones.
                cmlR.readChannelML(channel,params={},units=self.channelUnits)
            for synapse in channels.findall('.//{'+cml_ns+'}synapse_type'):
                cmlR.readSynapseML(synapse,units=self.channelUnits)
            for ionConc in channels.findall('.//{'+cml_ns+'}ion_concentration'):
                cmlR.readIonConcML(ionConc,units=self.channelUnits)

        #print "Loading cell definitions into MOOSE /library ..."
        mmlR = MorphML(self.nml_params)
        self.cellsDict = {}
        for cells in root_element.findall('.//{'+neuroml_ns+'}cells'):
            for cell in cells.findall('.//{'+neuroml_ns+'}cell'):
                cellDict = mmlR.readMorphML(cell,params={},lengthUnits=self.lengthUnits)
                self.cellsDict.update(cellDict)

        #print "Loading individual cells into MOOSE root ... "
        nmlR = NetworkML(self.nml_params)
        return nmlR.readNetworkML(root_element,self.cellsDict,params=params,lengthUnits=self.lengthUnits)

def loadNeuroML_L123(filename):
    neuromlR = NeuroML()
    return neuromlR.readNeuroMLFromFile(filename)

if __name__ == "__main__":
    if len(sys.argv)<2:
        print "You need to specify the neuroml filename."
        sys.exit(1)
    loadNeuroML_L123(sys.argv[1])
    
