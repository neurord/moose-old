#/**********************************************************************
#** This program is part of 'MOOSE', the
#** Messaging Object Oriented Simulation Environment.
#**           copyright (C) 2007 Upinder S. Bhalla. and NCBS
#** It is made available under the terms of the
#** GNU Lesser General Public License version 2.1
#** See the file COPYING.LIB for the full notice.
#**********************************************************************/

TARGET = _mesh.o

OBJ = \
	ChemCompt.o	\
	MeshCompt.o	\
	MeshEntry.o	\
	CubeMesh.o	\
	CylBase.o	\
	CylMesh.o	\
	NeuroNode.o	\
	NeuroMesh.o	\
	testMesh.o	\

HEADERS = \
	../basecode/header.h \


default: $(TARGET)

$(OBJ)	: $(HEADERS)
MeshEntry.o:	MeshEntry.h
Boundary.o:	Boundary.h
ChemCompt.o:	VoxelJunction.h ChemCompt.h MeshEntry.h Boundary.h
MeshCompt.o:	VoxelJunction.h ChemCompt.h MeshCompt.h MeshEntry.h Boundary.h
CylBase.o:	../utility/Vec.h CylBase.h
CylMesh.o:	../utility/Vec.h VoxelJunction.h ChemCompt.h CylBase.h CylMesh.h MeshEntry.h Boundary.h
CubeMesh.o:	VoxelJunction.h ChemCompt.h CubeMesh.h MeshEntry.h Boundary.h
NeuroNode.o: CylBase.h NeuroNode.h
NeuroMesh.o: ../basecode/SparseMatrix.h ChemCompt.h CylBase.h NeuroNode.h NeuroMesh.h
SpineMesh.o: ../basecode/SparseMatrix.h VoxelJunction.h ChemCompt.h CylBase.h NeuroNode.h NeuroMesh.h ../utility/Vec.h SpineEntry.h SpineMesh.h
SpineEntry.o: VoxelJunction.h ChemCompt.h CylBase.h ../utility/Vec.h SpineEntry.h
PsdMesh.o: ../basecode/SparseMatrix.h VoxelJunction.h ChemCompt.h CylBase.h ../utility/Vec.h SpineEntry.h SpineMesh.h
testMesh.o:	../basecode/SparseMatrix.h CylBase.h NeuroNode.h MeshEntry.h ChemCompt.h CylMesh.h Boundary.h

.cpp.o:
	$(CXX) $(CXXFLAGS) $(SMOLDYN_FLAGS) -I.. -I../basecode $< -c

$(TARGET):		$(OBJ) $(SMOLDYN_OBJ) $(HEADERS)
	$(LD) -r -o $(TARGET) $(OBJ) $(SMOLDYN_OBJ) $(SMOLDYN_LIB_PATH) $(SMOLDYN_LIBS) 

clean:
	-rm -f *.o $(TARGET) core core.*

