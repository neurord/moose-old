/**********************************************************************
** This program is part of 'MOOSE', the
** Messaging Object Oriented Simulation Environment.
**           Copyright (C) 2003-2014 Upinder S. Bhalla. and NCBS
** It is made available under the terms of the
** GNU Lesser General Public License version 2.1
** See the file COPYING.LIB for the full notice.
**********************************************************************/

#ifndef _KSOLVE_H
#define _KSOLVE_H

class Stoich;
class Ksolve: public ZombiePoolInterface
{
	public: 
		Ksolve();
		~Ksolve();

		//////////////////////////////////////////////////////////////////
		// Field assignment stuff
		//////////////////////////////////////////////////////////////////
		Id getStoich() const;
		void setStoich( Id stoich );

		unsigned int getNumLocalVoxels() const;
		unsigned int getNumAllVoxels() const;
		/**
		 * Assigns the number of voxels used in the entire reac-diff 
		 * system. Note that fewer than this may be used on any given node.
		 */
		void setNumAllVoxels( unsigned int num );

		/**
		 * Assigns number of different pools (chemical species) present in
		 * each voxel.
		 */
		void setNumAllPools( unsigned int num );

		/// Returns the vector of pool Num at the specified voxel.
		vector< double > getNvec( unsigned int voxel) const;
		void setNvec( unsigned int voxel, vector< double > vec );
		//////////////////////////////////////////////////////////////////
		// Dest Finfos
		//////////////////////////////////////////////////////////////////
		void process( const Eref& e, ProcPtr p );
		void reinit( const Eref& e, ProcPtr p );

		//////////////////////////////////////////////////////////////////
		// Solver interface functions
		//////////////////////////////////////////////////////////////////
		unsigned int getPoolIndex( const Eref& e ) const;
		unsigned int getVoxelIndex( const Eref& e ) const;
		
		//////////////////////////////////////////////////////////////////
		// ZombiePoolInterface inherited functions
		//////////////////////////////////////////////////////////////////

		void setN( const Eref& e, double v );
		double getN( const Eref& e ) const;
		void setNinit( const Eref& e, double v );
		double getNinit( const Eref& e ) const;
		void setDiffConst( const Eref& e, double v );
		double getDiffConst( const Eref& e ) const;

		/**
		 * Assigns number of different pools (chemical species) present in
		 * each voxel.
		 */
		void setNumPools( unsigned int num );
		unsigned int getNumPools() const;

		//////////////////////////////////////////////////////////////////
		static const Cinfo* initCinfo();
	private:
		/**
		 * Each VoxelPools entry handles all the pools in a single voxel.
		 * Each entry knows how to update itself in order to complete 
		 * the kinetic calculations for that voxel. The ksolver does
		 * multinode management by indexing only the subset of entries
		 * present on this node.
		 */
		vector< VoxelPools > pools_;

		/// First voxel indexed on the current node.
		unsigned int startVoxel_;

		/**
		 * Stoich is the class that sets up the reaction system and
		 * manages the stoichiometry matrix
		 */
		Id stoich_;

		/// Utility ptr used to help Pool Id lookups by the Ksolve.
		const Stoich* stoichPtr_;
};

#endif	// _KSOLVE_H
