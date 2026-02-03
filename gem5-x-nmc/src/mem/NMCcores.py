#  /*
#  * Copyright EPFL 2024
#  * Riselda Kodra
#  * Rafael Medina Morillas
#  *
#  */

from m5.SimObject import SimObject
# A wrapper for Near Memory Computing Cores
class NMCcores(SimObject):
    type = 'NMCcores'
    cxx_header = "mem/nmccores.hh"
    cxx_class = "NMCcores" 

