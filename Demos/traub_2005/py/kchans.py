# kchans.py --- 
# 
# Filename: kchans.py
# Description: 
# Author: subhasis ray
# Maintainer: 
# Created: Fri Apr 17 23:58:49 2009 (+0530)
# Version: 
# Last-Updated: Mon May 28 15:03:06 2012 (+0530)
#           By: subha
#     Update #: 1005
# URL: 
# Keywords: 
# Compatibility: 
# 
# 

# Commentary: 
# 
# 
# 
# 

# Change log:
# 
# 
# 
# 
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License as
# published by the Free Software Foundation; either version 3, or
# (at your option) any later version.
# 
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with this program; see the file COPYING.  If not, write to
# the Free Software Foundation, Inc., 51 Franklin Street, Fifth
# Floor, Boston, MA 02110-1301, USA.
# 
# 

# Code:

import moose
from channelbase import *
from numpy import where, linspace, exp, arange, ones, zeros, array
import numpy as np


class KChannel(ChannelBase):
    """This is a dummy base class to keep type information."""
    abstract = True
    Ek = -95e-3
    X = 0.0
    def __init__(self, path):
        ChannelBase.__init__(self, path)


class KDR(KChannel):
    """Delayed rectifier current

    `In hippocampal pyramidal neurons, however, it has been reported have relatively slow activation, with a time to peak of some 50-100 msec and even slower inactivation. Such a slow activation would make it ill suited to participate in the repolarization of the AP.... An equation that can describe IK(DR) in cortical neurons is
    
    IK(DR) = m^3 * h * gbar_K(DR) * (Vm - EK)
    
    where m and h depend on voltage and time.`
        - Johnston & Wu, Foundations of Cellular Neurophysiology (1995).

    But in Traub 2005, the equation used is:
    
    IK(DR) = m^4 * gbar_K(DR) * (Vm - EK)
    """
    abstract = False

    Xpower = 4
    tau_x = where(v_array < -10e-3, \
                      1e-3 * (0.25 + 4.35 * exp((v_array + 10.0e-3) / 10.0e-3)), \
                      1e-3 * (0.25 + 4.35 * exp((- v_array - 10.0e-3) / 10.0e-3)))
    inf_x = 1.0 / (1.0 + exp((- v_array - 29.5e-3) / 10e-3))

    def __init__(self, path):
        KChannel.__init__(self, path, xpower, ypower)


class KDR_FS(KDR):
    """KDR for fast spiking neurons"""
    abstract = False

    inf_x = 1.0 / (1.0 + exp((- v_array - 27e-3) / 11.5e-3))
    tau_x =  where(v_array < -10e-3, \
                       1e-3 * (0.25 + 4.35 * exp((v_array + 10.0e-3) / 10.0e-3)), \
                       1e-3 * (0.25 + 4.35 * exp((- v_array - 10.0e-3) / 10.0e-3)))

    def __init__(self, path):
        KChannel.__init__(self, path)


class KA(KChannel):
    """A type K+ channel"""
    abstract = False

    Xpower = 4
    Ypower = 1
    inf_x = 1 / ( 1 + exp( ( - v_array - 60e-3 ) / 8.5e-3 ) )
    tau_x =  1e-3 * (0.185 + 0.5 / ( exp( ( v_array + 35.8e-3 ) / 19.7e-3 ) + exp( ( - v_array - 79.7e-3 ) / 12.7e-3 ) ))
    inf_y =   1 / ( 1 + exp( ( v_array + 78e-3 ) / 6e-3 ) )
    tau_y = where( v_array <= -63e-3,\
                       1e-3 * 0.5 / ( exp( ( v_array + 46e-3 ) / 5e-3 ) + exp( ( - v_array - 238e-3 ) / 37.5e-3 ) ), \
                       9.5e-3)

    def __init__(self, path):
        KChannel.__init__(self, path)


class KA_IB(KA):
    """A type K+ channel for tufted intrinsically bursting cells -
    multiplies tau_h of KA by 2.6"""

    abstract = False

    tau_y = 2.6 * KA.tau_y

    def __init__(self, path):
        KChannel.__init__(self, path)


class K2(KChannel):
    Xpower = 1
    Ypower = 1
    inf_x = 1.0 / (1 + exp((-v_array - 10e-3) / 17e-3))
    tau_x = 1e-3 * (4.95 + 0.5 / (exp((v_array - 81e-3) / 25.6e-3) + \
                                      exp((-v_array - 132e-3) / 18e-3)))
    
    inf_y = 1.0 / (1 + exp((v_array + 58e-3) / 10.6e-3))
    tau_y = 1e-3 * (60 + 0.5 / (exp((v_array - 1.33e-3) / 200e-3) + \
					exp((-v_array - 130e-3) / 7.1e-3)))

    def __init__(self, path, Ek=-95e-3):
        KChannel.__init__(self, path)
	

class KM(KChannel):
    """Mascarinic sensitive K channel"""

    abstract = False

    Xpower = 1

    alpha_x =  1e3 * 0.02 / ( 1 + exp((-v_array - 20e-3 ) / 5e-3))
    beta_x = 1e3 * 0.01 * exp((-v_array - 43e-3) / 18e-3)

    def __init__(self, path):
        KChannel.__init__(self, path)

        
class KCaChannel(KChannel):
    """[Ca+2] dependent K+ channel base class."""

    abstract = True

    Zpower = 1

    def __init__(self, path, xpower=0.0, ypower=0.0, zpower=1.0, Ek=-95e-3):
        KChannel.__init__(self, path)


class KAHPBase(KCaChannel):
    abstract = True
    Z = 0.0

    def __init__(self, path):
        if moose.exists(path):
            KCaChannel.__init__(self, path, xpower=xpower, ypower=ypower, zpower=zpower, Ek=Ek)
            return
        KCaChannel.__init__(self, path, xpower=xpower, ypower=ypower, zpower=zpower, Ek=Ek)
        

class KAHP(KAHPBase):
    """AHP type K+ current"""
    abstract = False
    alpha_z = where(ca_conc < 100.0, 0.1 * ca_conc, 10.0)
    beta_z =  ones(ca_divs + 1) * 10.0

    def __init__(self, path):
        KAHPBase.__init__(self, path)
#         self.zGate.A.calcMode = 1
#         self.zGate.B.calcMode = 1


class KAHP_SLOWER(KAHPBase):
    abstract = False
    alpha_z = where(ca_conc < 500.0, 1e3 * ca_conc / 50000, 10.0)
    beta_z =  ones(ca_divs + 1) * 1.0

    def __init__(self, path):
        KAHPBase.__init__(self, path)
        # self.zGate.tableA.calcMode = 1
        # self.zGate.tableB.calcMode = 1


class KAHP_DP(KAHPBase):
    """KAHP for deep pyramidal cell"""
    abstract = False
    alpha_z = where(ca_conc < 100.0, 1e-1 * ca_conc, 10.0)
    beta_z =  ones(ca_divs + 1)
    def __init__(self, path):
        if moose.exists(path):
            KAHPBase.__init__(self, path)
            return
        KAHPBase.__init__(self, path)


class KC(KCaChannel):
    """C type K+ channel"""
    abstract = False

    Xpower = 1
    Zpower = 1
    tableA_z = where(ca_conc < 250.0, ca_conc / 250.0, 1.0)
    tableB_z = ones(ca_divs + 1)
    tableA_x = where(v_array < -10e-3, 
                      2e3 / 37.95 * ( exp( ( v_array * 1e3 + 50 ) / 11 - ( v_array * 1e3 + 53.5 ) / 27 ) ),
                      2e3 * exp(( - v_array * 1e3 - 53.5) / 27))
    tableB_x = where(v_array < -10e-3,
                   2e3 * exp(( - v_array * 1e3 - 53.5) / 27), 
                   0.0)
    instant = 4

    def __init__(self, path):
        KCaChannel.__init__(self, path)

        
class KC_FAST(KC):
    """Fast KC channel"""

    abstract = False

    tableA_x = KC.tableA_x * 2
    tableB_x = KC.tableB_x * 2

    def __init__(self, path):
        KC.__init__(self, path)

        
def initKChannelPrototypes(libpath='/library'):
    channel_names = ['KDR', 
                     'KDR_FS', 
                     'KA', 
                     'KA_IB',
                     'K2', 
                     'KM', 
                     'KAHP',
                     'KAHP_SLOWER',
                     'KAHP_DP',
                     'KC',
                     'KC_FAST']    
    return dict([(key, _prototypes[key]) for key in channel_names])
        

# 
# kchans.py ends here
