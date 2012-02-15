# squid.py --- 
# 
# Filename: squid.py
# Description: 
# Author: Subhasis Ray
# Maintainer: 
# Created: Mon Feb 13 11:35:11 2012 (+0530)
# Version: 
# Last-Updated: Tue Feb 14 14:14:47 2012 (+0530)
#           By: Subhasis Ray
#     Update #: 413
# URL: 
# Keywords: 
# Compatibility: 
# 
# 

# Commentary: 
# 
# Rewrite of the squid demo for new moose.
# 
# 

# Change log:
# 
# 
# 

# Code:

import numpy

import moose

GAS_CONSTANT = 8.314
FARADAY = 9.65e4
ABSOLUTE_ZERO = 273.15

def reversal_potential(temp, c_out, c_in):
    """Compute the reversal potential based on Nernst equation."""
    # NOTE the 70 mV added for compatibility with original HH
    return (GAS_CONSTANT * temp / FARADAY) * 1000.0 * numpy.log(c_out/c_in) + 70.0 + SquidModel.EREST_ACT

class SquidComp(moose.Compartment):
    """Compartment class enhanced with specific values set and
    calculated using dimensions."""
    def __init__(self, path):
        moose.Compartment.__init__(self, path)

    @property
    def xarea(self):
        """Area of cross section in cm^2 when length and diameter are in um"""
        return 1e-8 * numpy.pi * self.diameter * self.diameter / 4.0 # cm^2

    @property
    def area(self):
        """Area in cm^2 when length and diameter are in um"""
        return 1e-8 * self.length * numpy.pi * self.diameter # cm^2
    
    @property
    def specific_ra(self):
        return self.Ra * self.xarea / self.length
    @specific_ra.setter
    def specific_ra(self, value):
        self.Ra = value * self.length / self.xarea
        
    @property
    def specific_cm(self):
        return self.Cm * self.area
    @specific_cm.setter
    def specific_cm(self, value):
        self.Cm = value / self.area
    @property
    def specific_gl(self):
        return 1.0/(self.Rm * self.area)

    @specific_gl.setter
    def specific_gl(self, value):
        self.Rm = 1.0/(value * self.area)

    
    
class SquidModel(moose.Neutral):
    EREST_ACT = 0.0 # can be -70 mV if not following original HH convention
    VMIN = -30.0
    VMAX = 120.0
    VDIVS = 150
    Na_m_params = {'A_A':0.1 * (25.0 + EREST_ACT),
                   'A_B': -0.1,
                   'A_C': -1.0,
                   'A_D': -25.0 - EREST_ACT,
                   'A_F':-10.0,
                   'B_A': 4.0,
                   'B_B': 0.0,
                   'B_C': 0.0,
                   'B_D': 0.0 - EREST_ACT,
                   'B_F': -18.0}
    Na_h_params = {'A_A': 0.07,
                   'A_B': 0.0,
                   'A_C': 0.0,
                   'A_D': 0.0 - EREST_ACT,
                   'A_F': -20.0,
                   'B_A': 1.0,
                   'B_B': 0.0,
                   'B_C': 1.0,
                   'B_D': -30.0 - EREST_ACT,
                   'B_F': -10.0}
    K_n_params = {'A_A': 0.01*(10.0 + EREST_ACT),
                  'A_B': -0.01,
                  'A_C': -1.0,
                  'A_D': -10.0 - EREST_ACT,
                  'A_F': -10.0,
                  'B_A': 0.125,
                  'B_B': 0.0,
                  'B_C': 0.0,
                  'B_D': 0.0 - EREST_ACT,
                  'B_F': -80.0}
    
    def __init__(self, path):
        moose.Neutral.__init__(self, path)
        self.temperature = ABSOLUTE_ZERO + 6.3        
        self.K_out = 10.0
        self.Na_out = 460.0
        # Modified internal concentrations used to give HH values of
        # equilibrium constants from the Nernst equation at 6.3 deg C.
        # HH 1952a, p. 455        
        self.K_in = 301.4
        self.Na_in = 70.96
        self.Cl_out = 540.0        
        self.Cl_in = 100.0

        self.specific_gNa = 120.0 # mmho/cm^2
        self.specific_gK = 36.0 # mmho/cm^2

        self.squid_axon = SquidComp('%s/squid' % (self.path))
        self.squid_axon.length = 500 # um
        self.squid_axon.diameter = 500 # um
        self.squid_axon.Em = SquidModel.EREST_ACT + 10.613
        self.squid_axon.initVm = SquidModel.EREST_ACT        
        self.squid_axon.specific_cm = 1.0 # uF/cm^2
        self.squid_axon.specific_gl =  0.3 # mmho/cm^2
        self.squid_axon.specific_ra = 0.030 # kohm-cm
        
        self.Na_channel = self._create_HH_chan('%s/Na' % (self.squid_axon.path),
                                               self.VNa,
                                               self.specific_gNa,
                                               SquidModel.VDIVS,
                                               SquidModel.VMIN,
                                               SquidModel.VMAX,
                                               xpower=3.0,
                                               xparams = SquidModel.Na_m_params,
                                               ypower=1.0,
                                               yparams=SquidModel.Na_h_params)
                                               
                                               
        print 'Connected Na_channel:', moose.connect(self.Na_channel, 'channel', self.squid_axon, 'channel')
                                                
        self.K_channel = self._create_HH_chan('%s/K' % (self.squid_axon.path),
                                              self.VK,
                                              self.specific_gK,
                                              SquidModel.VDIVS,
                                              SquidModel.VMIN,
                                              SquidModel.VMAX,
                                              xpower=4.0,
                                              xparams=SquidModel.K_n_params)
        print 'Connected K channel:', moose.connect(self.K_channel, 'channel', self.squid_axon, 'channel')

        self.inject_delay = 50e-3
        self.inject_dur = 20e-3
        self.inject_amp = 0.1

        self.Vm_table = moose.Table('%s/Vm' % (self.path))
        print 'Connected Vm_table to compartment:', moose.connect(self.Vm_table, 'requestData', self.squid_axon, 'get_Vm')
        self.gK_table = moose.Table('%s/gK' % (self.path))
        moose.connect(self.gK_table, 'requestData', self.K_channel, 'get_Gk')

        
        
    @property
    def VK(self):
        """Reversal potential of K+ channels"""
        return reversal_potential(self.temperature, self.K_out, self.K_in)
    
    @property
    def VNa(self):
        """Reversal potential of Na+ channels"""
        return reversal_potential(self.temperature, self.Na_out, self.Na_in)

    def _create_HH_chan(self, path, Ek, specific_gbar, vdivs, vmin, vmax, xpower, xparams, ypower=0, yparams=None):
        """Create a HH Channel with gates specified by alphas and
        betas (list of A, B, C, D and F params)."""
        chan = moose.HHChannel('%s' % (path))
        chan.Gbar = specific_gbar * self.squid_axon.area
        chan.Xpower = xpower
        chan.Ek = Ek
        # chan.createGate('X')
        xgate = moose.HHGate('%s/gateX' % (chan.path))
        xgate.setupAlpha([xparams['A_A'],
                         xparams['A_B'],
                         xparams['A_C'],
                         xparams['A_D'],
                         xparams['A_F'],
                         xparams['B_A'],
                         xparams['B_B'],
                         xparams['B_C'],
                         xparams['B_D'],
                         xparams['B_F'],
                         vdivs, vmin, vmax])
        xgate.useInterpolation = True
        if ypower > 0:
            chan.Ypower = ypower
            # chan.createGate('Y')
            ygate = moose.HHGate('%s/gateY' % (chan.path))
            ygate.setupAlpha([yparams['A_A'],
                             yparams['A_B'],
                             yparams['A_C'],
                             yparams['A_D'],
                             yparams['A_F'],
                             yparams['B_A'],
                             yparams['B_B'],
                             yparams['B_C'],
                             yparams['B_D'],
                             yparams['B_F'],
                             vdivs, vmin, vmax])
            ygate.useInterpolation = True
        return chan
            
        
    
    def run(self, runtime, simdt=1e-6):
        moose.setClock(0, simdt)
        moose.setClock(1, simdt)
        moose.setClock(2, simdt)
        moose.setClock(3, simdt)
        moose.useClock(0, '%s/#[TYPE=Compartment]' % (self.path), 'init')
        moose.useClock(1, '%s/#[TYPE=Compartment]' % (self.path), 'process')
        moose.useClock(2, '%s/Na,%s/K' % (self.squid_axon.path, self.squid_axon.path), 'process')
        moose.useClock(3, '%s/#[TYPE=Table]' % (self.path), 'process')
        moose.reinit()
        print 'IN', self.squid_axon.path
        for msg in self.squid_axon.inMessages():
            print msg
        print 'OUT', self.squid_axon.path
        for msg in self.squid_axon.outMessages():
            print msg
        
        print 'IN', self.Na_channel.path
        for msg in self.Na_channel.inMessages():
            print msg
        print 'OUT', self.Na_channel.path
        for msg in self.Na_channel.outMessages():
            print msg
        moose.start(self.inject_delay)
        self.squid_axon.inject = self.inject_amp
        moose.start(self.inject_dur)
        rest = runtime - self.inject_delay - self.inject_dur
        if rest > 0:
            self.squid_axon.inject = 0.0
            moose.start(rest)

    def save_data(self):
        self.Vm_table.xplot('Vm.dat', 'Vm')
        self.gK_table.xplot('gK.dat', 'gK')


import pylab

if __name__ == '__main__':
    runtime = 100e-3
    simdt = 1e-6
    model = SquidModel('squid_demo')
    print model.VK, model.VNa
    model.run(runtime, simdt)
    model.save_data()
    pylab.plot(model.Vm_table.vec)
    # pylab.plot(model.gK_table.vec)
    pylab.show()
    
                                                                             
    
# 
# squid.py ends here
