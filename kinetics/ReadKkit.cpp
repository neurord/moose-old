/**********************************************************************
** This program is part of 'MOOSE', the
** Messaging Object Oriented Simulation Environment.
**           Copyright (C) 2003-2010 Upinder S. Bhalla. and NCBS
** It is made available under the terms of the
** GNU Lesser General Public License version 2.1
** See the file COPYING.LIB for the full notice.
**********************************************************************/


#include <iomanip>
#include <fstream>
#include "header.h"
#include "utility/utility.h"
#include "PoolBase.h"
#include "Pool.h"
#include "FuncPool.h"
#include "BufPool.h"
#include "ReacBase.h"
#include "EnzBase.h"
#include "lookupVolumeFromMesh.h"

#include "../shell/Shell.h"
#include "../shell/Wildcard.h"

#include "ReadKkit.h"

const double ReadKkit::EPSILON = 1.0e-15;
static const double KKIT_NA = 6.0e23; // Causes all sorts of conversion fun

unsigned int chopLine( const string& line, vector< string >& ret )
{
	ret.resize( 0 );
	stringstream ss( line );
	string arg;
	while ( ss >> arg ) {
            ret.push_back( trim(arg, "\"") );
	}
	return ret.size();
}

////////////////////////////////////////////////////////////////////////

ReadKkit::ReadKkit()
	:
	fastdt_( 0.001 ),
	simdt_( 0.01 ),
	controldt_( 0.1 ),
	plotdt_( 1 ),
	maxtime_( 1.0 ),
	transientTime_( 1.0 ),
	useVariableDt_( 0 ),
	defaultVol_( 1 ),
	version_( 11 ),
	initdumpVersion_( 3 ),
	moveOntoCompartment_( true ),
	numCompartments_( 0 ),
	numPools_( 0 ),
	numReacs_( 0 ),
	numEnz_( 0 ),
	numMMenz_( 0 ),
	numPlot_( 0 ),
	numStim_( 0 ),
	numOthers_( 0 ),
	lineNum_( 0 ),
	shell_( reinterpret_cast< Shell* >( Id().eref().data() ) )
{
	;
}

//////////////////////////////////////////////////////////////////
// Fields for readKkit
//////////////////////////////////////////////////////////////////

double ReadKkit::getMaxTime() const
{
	return maxtime_;
}

double ReadKkit::getPlotDt() const
{
	return plotdt_;
}

double ReadKkit::getDefaultVol() const
{
	return defaultVol_;
}

string ReadKkit::getBasePath() const
{
	return basePath_;
}

unsigned int ReadKkit::getVersion() const
{
	return version_;
}

bool ReadKkit::getMoveOntoCompartment() const
{
	return moveOntoCompartment_;
}

void ReadKkit::setMoveOntoCompartment( bool v )
{
	moveOntoCompartment_ = v;
}

//////////////////////////////////////////////////////////////////
// The read functions.
//////////////////////////////////////////////////////////////////
Id  makeStandardElements( Id pa, const string& modelname )
{
	Shell* shell = reinterpret_cast< Shell* >( Id().eref().data() );
	cout << " kkit read " << pa << " " << modelname << " "<< MooseGlobal;
	Id mgr = shell->doCreate( "Neutral", pa, modelname, 1, MooseGlobal );
	Id kinetics = 
		shell->doCreate( "CubeMesh", mgr, "kinetics", 1,  MooseGlobal );
		SetGet2< double, unsigned int >::set( kinetics, "buildDefaultMesh", 1e-15, 1 );
	assert( kinetics != Id() );

	Id graphs = shell->doCreate( "Neutral", mgr, "graphs", 1, MooseGlobal);
	assert( graphs != Id() );
	Id moregraphs = shell->doCreate( "Neutral", mgr, "moregraphs", 1, MooseGlobal );

	Id geometry = shell->doCreate( "Neutral", mgr, "geometry", 1, MooseGlobal );
	assert( geometry != Id() );

	Id groups = 
		shell->doCreate( "Neutral", mgr, "groups", 1, MooseGlobal );
	assert( groups != Id() );
	return mgr;
}
/**
 * The readcell function implements the old GENESIS cellreader
 * functionality. Although it is really a parser operation, I
 * put it here in Kinetics because the cell format is independent
 * of parser and is likely to remain a legacy for a while.
 */
Id ReadKkit::read(
	const string& filename, 
	const string& modelname,
	Id pa, const string& method )
{
	ifstream fin( filename.c_str() );
	if (!fin){
		cerr << "ReadKkit::read: could not open file " << filename << endl;
		return Id();
    }

	if ( method.substr(0, 5) == "multi" ) {
		moveOntoCompartment_ = true;
	}

	Shell* s = reinterpret_cast< Shell* >( ObjId().data() );
	Id mgr = makeStandardElements( pa, modelname );
	assert( mgr != Id() );

	baseId_ = mgr;
	basePath_ = mgr.path();
	enzCplxMols_.resize( 0 );

	innerRead( fin );

	assignPoolCompartments();
	assignReacCompartments();
	assignEnzCompartments();
	assignMMenzCompartments();

	convertParametersToConcUnits();

	s->doSetClock( 4, simdt_ );
	s->doSetClock( 8, plotdt_ );
	
	string simpath = basePath_ + "/kinetics/##[]";
	s->doUseClock( simpath, "process", 4 );
	string plotpath = basePath_ + "/graphs/##[TYPE=Table]," +
			basePath_ + "/moregraphs/##[TYPE=Table]";
	s->doUseClock( plotpath, "process", 8 );

	/*
	s->setCwe( mgr );
	Field< double >::set( mgr, "plotDt", plotdt_ );
	Field< double >::set( mgr, "simDt", simdt_ );
	Field< double >::set( mgr, "runTime", maxtime_ );
	Field< double >::set( mgr, "version", version_ );
	SetGet1< string >::set( mgr, "build", method );
	*/
	s->doReinit();
	return mgr;
}

void ReadKkit::run()
{
	shell_->doSetClock( 4, simdt_ );
	shell_->doSetClock( 5, simdt_ );
	shell_->doSetClock( 8, plotdt_ );
	shell_->doSetClock( 7, 0 );
	string poolpath = basePath_ + "/kinetics/##[ISA=Pool]";
	string reacpath = basePath_ + "/kinetics/##[ISA!=Pool]";
	string plotpath = basePath_ + "/graphs/##[TYPE=Table]," + 
		basePath_ + "/moregraphs/##[TYPE=Table]";
	shell_->doUseClock( reacpath, "process", 4 );
	shell_->doUseClock( poolpath, "process", 5 );
	shell_->doUseClock( plotpath, "process", 8 );
	shell_->doReinit();
	if ( useVariableDt_ ) {
		shell_->doSetClock( 4, fastdt_ );
		shell_->doSetClock( 5, fastdt_ );
		shell_->doStart( transientTime_ );
		shell_->doSetClock( 4, simdt_ );
		shell_->doSetClock( 5, simdt_ );
		shell_->doStart( maxtime_ - transientTime_ );
	} else {
		shell_->doStart( maxtime_ );
	}
}

void ReadKkit::dumpPlots( const string& filename )
{
	// ofstream fout ( filename.c_str() );
	vector< ObjId > plots;
	string plotpath = basePath_ + "/graphs/##[TYPE=Table]," + 
		basePath_ + "/moregraphs/##[TYPE=Table]";
	wildcardFind( plotpath, plots );
	for ( vector< ObjId >::iterator 
					i = plots.begin(); i != plots.end(); ++i )
		SetGet2< string, string >::set( *i, "xplot",
			filename, i->element()->getName() );
}

void ReadKkit::innerRead( ifstream& fin )
{
	string line;
	string temp;
	lineNum_ = 0;
	string::size_type pos;
	bool clearLine = 1;
	ParseMode parseMode = INIT;

	while ( getline( fin, temp ) ) {
		lineNum_++;
		if ( clearLine )
			line = "";
                temp = trim(temp);
		if ( temp.length() == 0 )
				continue;
		pos = temp.find_last_not_of( "\t " );
		if ( pos == string::npos ) { 
			// Nothing new in line, go with what was left earlier, 
			// and clear out line for the next cycle.
			temp = "";
			clearLine = 1;
		} else {
			if ( temp[pos] == '\\' ) {
				temp[pos] = ' ';
				line.append( temp );
				clearLine = 0;
				continue;
			} else {
				line.append( temp );
				clearLine = 1;
			}
		}
			
		pos = line.find_first_not_of( "\t " );
		if ( pos == string::npos )
				continue;
		else
			line = line.substr( pos );
		if ( line.substr( 0, 2 ) == "//" )
				continue;
		if ( (pos = line.find("//")) != string::npos ) 
			line = line.substr( 0, pos );
		if ( line.substr( 0, 2 ) == "/*" ) {
				parseMode = COMMENT;
				line = line.substr( 2 );
		}

		if ( parseMode == COMMENT ) {
			pos = line.find( "*/" );
			if ( pos != string::npos ) {
				parseMode = DATA;
				if ( line.length() > pos + 2 )
					line = line.substr( pos + 2 );
			}
		}

		if ( parseMode == DATA )
				readData( line );
		else if ( parseMode == INIT ) {
				parseMode = readInit( line );
		}
	}
	
	/*
	cout << " innerRead: " <<
			lineNum_ << " lines read, " << 
			numCompartments_ << " compartments, " << 
			numPools_ << " molecules, " << 
			numReacs_ << " reacs, " << 
			numEnz_ << " enzs, " << 
			numMMenz_ << " MM enzs, " << 
			numOthers_ << " others," <<
			numPlot_ << " plots," <<
			" PlotDt = " << plotdt_ <<
			endl;
			*/
}

ReadKkit::ParseMode ReadKkit::readInit( const string& line )
{
	vector< string > argv;
	chopLine( line, argv ); 
	if ( argv.size() < 3 )
		return INIT;

	if ( argv[0] == "FASTDT" ) {
		fastdt_ = atof( argv[2].c_str() );
		return INIT;
	}
	if ( argv[0] == "SIMDT" ) {
		simdt_ = atof( argv[2].c_str() );
		return INIT;
	}
	if ( argv[0] == "CONTROLDT" ) {
		controldt_ = atof( argv[2].c_str() );
		return INIT;
	}
	if ( argv[0] == "PLOTDT" ) {
		plotdt_ = atof( argv[2].c_str() );
		return INIT;
	}
	if ( argv[0] == "MAXTIME" ) {
		maxtime_ = atof( argv[2].c_str() );
		return INIT;
	}
	if ( argv[0] == "TRANSIENT_TIME" ) {
		transientTime_ = atof( argv[2].c_str() );
		return INIT;
	}
	if ( argv[0] == "VARIABLE_DT_FLAG" ) {
		useVariableDt_ = atoi( argv[2].c_str() );
		return INIT;
	}
	if ( argv[0] == "DEFAULT_VOL" ) {
		defaultVol_ = atof( argv[2].c_str() );
		return INIT;
	}
	if ( argv[0] == "VERSION" ) {
		version_ = atoi( argv[2].c_str() );
		return INIT;
	}

	if ( argv[0] == "initdump" ) {
		initdumpVersion_ = atoi( argv[2].c_str() );
		return DATA;
	}

	return INIT;
}


void ReadKkit::readData( const string& line )
{
	vector< string > argv;
	chopLine( line, argv ); 
	
	if ( argv[0] == "simundump" )
		undump( argv );
	else if ( argv[0] == "addmsg" )
		addmsg( argv );
	else if ( argv[0] == "call" )
		call( argv );
	else if ( argv[0] == "simobjdump" )
		objdump( argv );
	else if ( argv[0] == "xtextload" )
		textload( argv );
	else if ( argv[0] == "loadtab" )
		loadTab( argv );
}

string ReadKkit::pathTail( const string& path, string& head ) const
{
	string::size_type pos = path.find_last_of( "/" );
	assert( pos != string::npos );

	head = basePath_ + path.substr( 0, pos ); 
	return path.substr( pos + 1 );
}

string ReadKkit::cleanPath( const string& path ) const
{
	// Could surely do this better with STL. But harder to understand.
	// Harsha: Cleaned up this function
	// minus was getting replaced with underscore, but in some genesis model
	// pool and reaction/Enzyme has same string name with 
	// difference of minus and underscore like eIF4G_A-clx and eIF4G_A_clx,
	// which later created a problem as the same name exist in moose when minus
	// was replaced with underscore.
	//So replacing minus with _minus_ like I do in SBML 
	string ret = path;
	string cleanString;
	for ( unsigned int i = 0; i < path.length(); ++i ) {
		char c = ret[i];
		if ( c == '*' )
			cleanString += "_p";
		else if ( c == '[' || c == ']' || c == '@' || c == ' ')
			cleanString += '_';
		else if (c == '-')
			cleanString += "_dash_";
		else
			cleanString += c;
	}
	return cleanString;
}

void assignArgs( map< string, int >& argConv, const vector< string >& args )
{
	for ( unsigned int i = 2; i != args.size(); ++i )
		argConv[ args[i] ] = i + 2;
}

void ReadKkit::objdump( const vector< string >& args)
{
	if ( args[1] == "kpool" )
		assignArgs( poolMap_, args );
	else if ( args[1] == "kreac" )
		assignArgs( reacMap_, args );
	else if ( args[1] == "kenz" )
		assignArgs( enzMap_, args );
	else if ( args[1] == "group" )
		assignArgs( groupMap_, args );
	else if ( args[1] == "xtab" )
		assignArgs( tableMap_, args );
	else if ( args[1] == "stim" )
		assignArgs( stimMap_, args );
}

void ReadKkit::call( const vector< string >& args)
{
	/// call /kinetics/foo/notes LOAD notes_string_here
	if ( args.size() > 3 ) {
		unsigned int len = args[1].length();
		if ( ( args[1].substr( len - 5 ) ==  "notes" ) &&
			args[2] == "LOAD" ) {
			if ( args[3].length() == 0 )
				return;
			//HARSHA: Added CleanPath.
			string objName = cleanPath(args[1].substr( 0, len - 5 ));
		        Id test(basePath_+objName);
			Id obj( basePath_ + objName + "info" );
			if ( obj != Id() ) {
				string notes = "";
				string space = "";
				for ( unsigned int i = 3; i < args.size(); ++i ) {
					unsigned int innerLength = args[i].length();
					if ( innerLength == 0 )
						continue;
					unsigned int start = 0;
					unsigned int end = innerLength;
					if ( args[i][0] == '\"' )
						start = 1;
					if ( args[i][innerLength - 1] == '\"' )
						end = innerLength - 1 - start;

					notes += space + args[i].substr( start, end );
					space = " ";
				}
				bool OK = Field< string >::set( obj, "notes", notes );
				assert( OK );
			}
		}
	}
}

void ReadKkit::textload( const vector< string >& args)
{
}

void ReadKkit::undump( const vector< string >& args)
{
	if ( args[1] == "kpool" )
		buildPool( args );
	else if ( args[1] == "kreac" )
		buildReac( args );
	else if ( args[1] == "kenz" )
		buildEnz( args );
	else if ( args[1] == "text" )
		buildText( args );
	else if ( args[1] == "xplot" )
		buildPlot( args );
	else if ( args[1] == "xgraph" )
		buildGraph( args );
	else if ( args[1] == "group" )
		buildGroup( args );
	else if ( args[1] == "geometry" )
		buildGeometry( args );
	else if ( args[1] == "stim" )
		buildStim( args );
	else if ( args[1] == "xcoredraw" )
		;
	else if ( args[1] == "xtree" )
		;
	else if ( args[1] == "xtext" )
		;
	else if ( args[1] == "doqcsinfo" )
		;
	else if ( args[1] == "kchan" )
		buildChan( args );
	else if ( args[1] == "xtab" )
		buildTable( args );
	else
		cout << "ReadKkit::undump: Do not know how to build '" << args[1] <<
		"'\n";
}

Id ReadKkit::buildCompartment( const vector< string >& args )
{
	Id compt;
	numCompartments_++;
	return compt;
}

Id ReadKkit::buildReac( const vector< string >& args )
{
	string head;
	string clean = cleanPath( args[2] );
	string tail = pathTail( clean, head );
	Id pa = shell_->doFind( head ).id;
	assert( pa != Id() );

	double kf = atof( args[ reacMap_[ "kf" ] ].c_str() );
	double kb = atof( args[ reacMap_[ "kb" ] ].c_str() );

	// We have a slight problem because MOOSE has a more precise value for
	// NA than does kkit. Here we assume that the conc units from Kkit are
	// meant to be OK, so they override the #/cell (lower case k) units.
	// So we convert all the Kfs and Kbs in the entire system after
	// the model has been created, once we know the order of each reac.

	Id reac = shell_->doCreate( "Reac", pa, tail, 1 );
	reacIds_[ clean.substr( 10 ) ] = reac; 
	// Here is another hack: The native values stored in the reac are
	// Kf and Kb, in conc units. However the 'clean' values from kkit
	// are the number values numKf and numKb. In the 
	// function convertReacRatesToNumUnits we take the numKf and numKb and
	// do proper conc scaling.
	Field< double >::set( reac, "Kf", kf );
	Field< double >::set( reac, "Kb", kb );

	Id info = buildInfo( reac, reacMap_, args );
	numReacs_++;
	return reac;
}

void ReadKkit::separateVols( Id pool, double vol )
{
	static const double TINY = 1e-3;

	for ( unsigned int i = 0 ; i < vols_.size(); ++i ) {
		if ( fabs( vols_[i] - vol ) / ( fabs( vols_[i] ) + fabs( vol ) ) < TINY ) {
			volCategories_[i].push_back( pool );
			return;
		}
	}
	vols_.push_back( vol );
	vector< Id > temp( 1, pool );
	volCategories_.push_back( temp );
}

bool volCompare( 
				const pair< unsigned int, double >& A,
				const pair< unsigned int, double >& B )
{
	return A.second < B.second;
}
// Returns the ranking of each volume. Largest is 0. This would be the
// suffix to attach to the compartment name.
vector< unsigned int > findVolOrder( const vector< double >& vols ) 
{
	vector< pair< unsigned int, double > > p( vols.size() );
	for ( unsigned int i = 0; i < vols.size(); ++i ) {
		p[i].first = i;
		p[i].second = vols[i];
	}
	sort( p.begin(), p.end(), volCompare );
	vector< unsigned int > ret( vols.size() );
	for ( unsigned int i = 0; i < vols.size(); ++i ) {
		ret[ vols.size() -i -1 ] = p[i].first;
	}
	return ret;
}

// We assume that the biggest compartment contains all the rest.
// This is not true in synapses, where they are adjacent.
void ReadKkit::assignPoolCompartments()
{
	Id kinetics = Neutral::child( baseId_.eref(), "kinetics" );
	assert( kinetics != Id() );
	vector< unsigned int > volOrder = findVolOrder( vols_ );
	assert( volCategories_.size() == vols_.size() );
	assert( volCategories_.size() == volOrder.size() );

	for ( unsigned int j = 0 ; j < volOrder.size(); ++j ) {
		unsigned int i = volOrder[j];
		if ( vols_[i] < 0 ) // Special case for enz complexes: vol == -1.
			continue;
		string name;
		Id kinId = Neutral::child( baseId_.eref(), "kinetics" );
		assert( kinId != Id() );
		Id comptId;
		// if ( i == maxi )
		if ( j == 0 ) {
			comptId = kinId;
		} else {
			stringstream ss;
			ss << "compartment_" << j;
			name = ss.str();
			comptId = shell_->doCreate( "CubeMesh", baseId_, name, 1 ) ;
		}
		Id meshId = Neutral::child( comptId.eref(), "mesh" );
		assert( meshId != Id() );
		double side = pow( vols_[i], 1.0 / 3.0 );
		vector< double > coords( 9, side );
		coords[0] = coords[1] = coords[2] = 0;
		// Field< double >::set( comptId, "volume", vols_[i] );
		Field< vector< double > >::set( comptId, "coords", coords );
		// compartments_.push_back( comptId );
		for ( vector< Id >::iterator j = volCategories_[i].begin();
			j != volCategories_[i].end(); ++j ) {
			if ( moveOntoCompartment_ ) {
				if ( ! (getCompt( *j ).id == comptId ) )
					shell_->doMove( *j, comptId );
			}
		}
	}
}

Id findParentComptOfReac( Id reac )
{
	static const Finfo* subFinfo =
		   	ReacBase::initCinfo()->findFinfo( "subOut" );
	assert( subFinfo );

		vector< Id > subVec;
		unsigned int numSub = 
				reac.element()->getNeighbours( subVec, subFinfo );
		assert( numSub > 0 );
		// For now just put the reac in the compt belonging to the 
		// first substrate
		return getCompt( subVec[0] );
}

/**
 * Goes through all Reacs and connects them up to each of the compartments
 * in which one or more of their reactants resides.
 * Thus, if any of these compartments changes volume, the Reac will
 * be informed.
 */
void ReadKkit::assignReacCompartments()
{
	for ( map< string, Id >::iterator i = reacIds_.begin(); 
		i != reacIds_.end(); ++i ) {
		Id compt = findParentComptOfReac( i->second );
		if ( moveOntoCompartment_ ) {
			if ( ! (getCompt( i->second ).id == compt ) )
				shell_->doMove( i->second, compt );
		}
	}
}


/**
 * Return the MeshEntry into which the enzyme should be placed.
 * This is simple: Just identify the compartment holding the enzyme
 * molecule.
 */
Id findMeshOfEnz( Id enz )
{
	static const Finfo* enzFinfo =
		   	EnzBase::initCinfo()->findFinfo( "enzOut" );
	assert( enzFinfo );

		vector< Id > enzVec;
		unsigned int numEnz = 
				enz.element()->getNeighbours( enzVec, enzFinfo );
		assert( numEnz == 1 );
		vector< Id > meshEntries;
		return getCompt( enzVec[0] );
}

/**
 * Goes through all Enzs and connects them up to each of the compartments
 * in which one or more of their reactants resides.
 * Thus, if any of these compartments changes volume, the Enz will
 * be informed.
 */
void ReadKkit::assignEnzCompartments()
{;
}

/**
 * Goes through all MMenzs and connects them up to each of the compartments
 * in which one or more of their reactants resides.
 * Thus, if any of these compartments changes volume, the MMenz will
 * be informed.
 */
void ReadKkit::assignMMenzCompartments()
{;
}

Id ReadKkit::buildEnz( const vector< string >& args )
{
	string head;
	string clean = cleanPath( args[2] );
	string tail = pathTail( clean, head );
	Id pa = shell_->doFind( head ).id;
	assert ( pa != Id() );

	double k1 = atof( args[ enzMap_[ "k1" ] ].c_str() );
	double k2 = atof( args[ enzMap_[ "k2" ] ].c_str() );
	double k3 = atof( args[ enzMap_[ "k3" ] ].c_str() );
	// double volscale = atof( args[ enzMap_[ "vol" ] ].c_str() );
	double nComplexInit = 
		atof( args[ enzMap_[ "nComplexInit" ] ].c_str() );
	// double vol = atof( args[ enzMap_[ "vol" ] ].c_str());
	bool isMM = atoi( args[ enzMap_[ "usecomplex" ] ].c_str());
	assert( poolVols_.find( pa ) != poolVols_.end() );
	// double vol = poolVols_[ pa ];
	
	/**
	 * vsf is vol scale factor, which is what GENESIS stores in 'vol' field
	 * n = vsf * conc( uM )
	 * Also, n = ( conc (uM) / 1e6 ) * NA * vol
	 * so, vol = 1e6 * vsf / NA
	 */

	if ( isMM ) {
		Id enz = shell_->doCreate( "MMenz", pa, tail, 1 );
		assert( enz != Id () );
		string mmEnzPath = clean.substr( 10 );
		mmEnzIds_[ mmEnzPath ] = enz; 

		assert( k1 > EPSILON );
		double Km = ( k2 + k3 ) / k1;

		Field< double >::set( enz, "Km", Km );
		Field< double >::set( enz, "kcat", k3 );
		Id info = buildInfo( enz, enzMap_, args );
		numMMenz_++;
		return enz;
	} else {
		Id enz = shell_->doCreate( "Enz", pa, tail, 1 );
		// double parentVol = Field< double >::get( pa, "volume" );
		assert( enz != Id () );
		string enzPath = clean.substr( 10 );
		enzIds_[ enzPath ] = enz; 

		// Need to figure out what to do about these. Perhaps it is OK
		// to do this assignments in raw #/cell units.
		Field< double >::set( enz, "k3", k3 );
		Field< double >::set( enz, "k2", k2 );
		Field< double >::set( enz, "k1", k1 );

		string cplxName = tail + "_cplx";
		string cplxPath = enzPath + "/" + cplxName;
		Id cplx = shell_->doCreate( "Pool", enz, cplxName, 1 );
		assert( cplx != Id () );
		poolIds_[ cplxPath ] = cplx; 
		// Field< double >::set( cplx, "nInit", nComplexInit );
		Field< double >::set( cplx, "nInit", nComplexInit );

		// Use this later to assign mesh entries to enz cplx.
		enzCplxMols_.push_back( pair< Id, Id >(  pa, cplx ) );
		separateVols( cplx, -1 ); // -1 vol is a flag to defer mesh assignment for the cplx pool.

		ObjId ret = shell_->doAddMsg( "OneToAll", 
			ObjId( enz, 0 ), "cplx",
			ObjId( cplx, 0 ), "reac" ); 
		assert( ret != ObjId() );

		// cplx()->showFields();
		// enz()->showFields();
		// pa()->showFields();
		Id info = buildInfo( enz, enzMap_, args );
		numEnz_++;
		return enz;
	}
}

Id ReadKkit::buildText( const vector< string >& args )
{
	Id text;
	numOthers_++;
	return text;
}

Id ReadKkit::buildInfo( Id parent, 
	map< string, int >& m, const vector< string >& args )
{
	Id info = shell_->doCreate( "Annotator", parent, "info", 1 );
	assert( info != Id() );

	double x = atof( args[ m[ "x" ] ].c_str() );
	double y = atof( args[ m[ "y" ] ].c_str() );

	Field< double >::set( info, "x", x );
	Field< double >::set( info, "y", y );
	Field< string >::set( info, "color", args[ m[ "xtree_fg_req" ] ] );
	Field< string >::set( info, "textColor", 
		args[ m[ "xtree_textfg_req" ] ] );

	return info;
}

Id ReadKkit::buildGroup( const vector< string >& args )
{
	string head;
	string tail = pathTail( cleanPath( args[2] ), head );

	Id pa = shell_->doFind( head ).id;
	assert( pa != Id() );
	Id group = shell_->doCreate( "Neutral", pa, tail, 1 );
	assert( group != Id() );
	Id info = buildInfo( group, groupMap_, args );

	numOthers_++;
	return group;
}

/**
 * There is a problem with this conversion, because of the discrepancy of
 * the correct NA and the version (6e23) used in kkit. I take the 
 * concentration as authoritative, not the # of molecules. This is because
 * I use conc units for the rates as well, and to use n for pools and
 * conc for reactions will always introduce errors. This is still not
 * a great solution, because, for example, simulations involving receptor
 * traffic are originally framed in terms of number of receptors, not conc.
 */
Id ReadKkit::buildPool( const vector< string >& args )
{
	string head;
	string clean = cleanPath( args[2] );
	string tail = pathTail( clean, head );
	Id pa = shell_->doFind( head ).id;
	assert( pa != Id() );

	double nInit = atof( args[ poolMap_[ "nInit" ] ].c_str() );
	double vsf = atof( args[ poolMap_[ "vol" ] ].c_str() ); 
	/**
	 * vsf is vol scale factor, which is what GENESIS stores in 'vol' field
	 * n = vsf * conc( uM )
	 * Also, n = ( conc (uM) / 1e6 ) * NA * vol
	 * so, vol = 1e6 * vsf / NA
	 */
	double vol = 1.0e3 * vsf / KKIT_NA; // Converts volscale to actual vol in m^3
	int slaveEnable = atoi( args[ poolMap_[ "slave_enable" ] ].c_str() );
	double diffConst = atof( args[ poolMap_[ "DiffConst" ] ].c_str() );

	// I used -ve D as a flag to replace pool-specific D 
	// with the global value of D. Here we just ignore it.
	if ( diffConst < 0 ) 
		diffConst = 0;

	Id pool;
	if ( slaveEnable == 0 ) {
		pool = shell_->doCreate( "Pool", pa, tail, 1 );
	} else if ( slaveEnable & 4 ) {
		pool = shell_->doCreate( "BufPool", pa, tail, 1 );
	} else {
		pool = shell_->doCreate( "Pool", pa, tail, 1 );
		/*
		cout << "ReadKkit::buildPool: Unknown slave_enable flag '" << 
			slaveEnable << "' on " << clean << "\n";
			*/
		poolFlags_[pool] = slaveEnable;
	}
	assert( pool != Id() );
	// skip the 10 chars of "/kinetics/"
	poolIds_[ clean.substr( 10 ) ] = pool; 

	Field< double >::set( pool, "nInit", nInit );
	Field< double >::set( pool, "diffConst", diffConst );
	// SetGet1< double >::set( pool, "setVolume", vol );
	separateVols( pool, vol );
	poolVols_[pool] = vol;

	Id info = buildInfo( pool, poolMap_, args );

	/*
	cout << setw( 20 ) << head << setw( 15 ) << tail << "	" << 
		setw( 12 ) << nInit << "	" << 
		vol << "	" << diffConst << "	" <<
		slaveEnable << endl;
		*/
	numPools_++;
	return pool;
}

/**
 * Finds the source pool for a SumTot. It also deals with cases where 
 * the source is an enz-substrate complex
 */
Id ReadKkit::findSumTotSrc( const string& src )
{
	map< string, Id >::iterator i = poolIds_.find( src );
	if ( i != poolIds_.end() ) { 
		return i->second;
	}
	i = enzIds_.find( src );
	if ( i != enzIds_.end() ) {
		string head;
		string cplx = src + '/' + pathTail( src, head ) + "_cplx";
		i = poolIds_.find( cplx );
		if ( i != poolIds_.end() ) { 
			return i->second;
		}
	}
	cout << "Error: ReadKkit::findSumTotSrc: Cannot find source pool '" << src << endl;
	assert( 0 );
	return Id();
}

void ReadKkit::buildSumTotal( const string& src, const string& dest )
{
	map< string, Id >::iterator i = poolIds_.find( dest );
	assert( i != poolIds_.end() );
	Id destId = i->second;
	
	// Don't bother on buffered pool.
	if ( destId.element()->cinfo()->isA( "BufPool" ) ) 
		return;

	Id sumId;
	// Check if the pool has not yet been converted to handle SumTots.
	if ( destId.element()->cinfo()->name() == "Pool" ) {
		sumId = shell_->doCreate( "SumFunc", destId, "func", 1 );
		// Turn dest into a FuncPool.
		destId.element()->zombieSwap( FuncPool::initCinfo() );

		ObjId ret = shell_->doAddMsg( "single", 
			ObjId( sumId, 0 ), "output",
			ObjId( destId, 0 ), "input" ); 
		assert( ret != ObjId() );
	} else {
		sumId = Neutral::child( destId.eref(), "func" );
	}

	if ( sumId == Id() ) {
		cout << "Error: ReadKkit::buildSumTotal: could not make SumFunc on '"
		<< dest << "'\n";
		return;
	}
	
	Id srcId = findSumTotSrc( src );

	ObjId ret = shell_->doAddMsg( "single", 
		ObjId( srcId, 0 ), "nOut",
		ObjId( sumId, 0 ), "input" ); 
	assert( ret != ObjId() );

}

/**
 * Build a Stim entry in simulation, i.e., a PulseGen
 */
Id ReadKkit::buildStim( const vector< string >& args )
{
	string head;
	string clean = cleanPath( args[2] );
	string tail = pathTail( clean, head );
	Id pa = shell_->doFind( head ).id;
	assert( pa != Id() );

	double level1 = atof( args[ stimMap_[ "firstLevel" ] ].c_str() ); 
	double width1 = atof( args[ stimMap_[ "firstWidth" ] ].c_str() ); 
	double delay1 = atof( args[ stimMap_[ "firstDelay" ] ].c_str() ); 
	double level2 = atof( args[ stimMap_[ "secondLevel" ] ].c_str() ); 
	double width2 = atof( args[ stimMap_[ "secondWidth" ] ].c_str() ); 
	double delay2 = atof( args[ stimMap_[ "secondLevel" ] ].c_str() ); 
	double baselevel = atof( args[ stimMap_[ "baseLevel" ] ].c_str() );

	Id stim = shell_->doCreate( "PulseGen", pa, tail, 1 );
	assert( stim != Id() );
	string stimPath = clean.substr( 10 );
	stimIds_[ stimPath ] = stim;
	Field< double >::set( stim, "firstLevel", level1 );
	Field< double >::set( stim, "firstWidth", width1 );
	Field< double >::set( stim, "firstDelay", delay1 );
	Field< double >::set( stim, "secondLevel", level2 );
	Field< double >::set( stim, "secondWidth", width2 );
	Field< double >::set( stim, "secondDelay", delay2 );
	Field< double >::set( stim, "baseLevel", baselevel );

	numStim_++;
	return stim;
}

/**
 * Build a Kchan entry in simulation. For now a dummy Neutral.
 */
Id ReadKkit::buildChan( const vector< string >& args )
{
	string head;
	string clean = cleanPath( args[2] );
	string tail = pathTail( clean, head );
	Id pa = shell_->doFind( head ).id;
	assert( pa != Id() );

	cout << "Warning: Kchan not yet supported in MOOSE, creating dummy:\n"
		<< "	" << clean << "\n";
	Id chan = shell_->doCreate( "Neutral", pa, tail, 1 );
	assert( chan != Id() );
	string chanPath = clean.substr( 10 );
	chanIds_[ chanPath ] = chan;
	return chan;
}
	

Id ReadKkit::buildGeometry( const vector< string >& args )
{
	Id geometry;
	numOthers_++;
	return geometry;
}

Id ReadKkit::buildGraph( const vector< string >& args )
{
	string head;
	string tail = pathTail( cleanPath( args[2] ), head );

	Id pa = shell_->doFind( head ).id;
	assert( pa != Id() );
	Id graph = shell_->doCreate( "Neutral", pa, tail, 1 );
	assert( graph != Id() );
	numOthers_++;
	return graph;
}

Id ReadKkit::buildPlot( const vector< string >& args )
{
	string head;
	string tail = pathTail( cleanPath( args[2] ), head ); // Name of plot
	string temp;
	string graph = pathTail( head, temp ); // Name of graph

	Id pa = shell_->doFind( head ).id;
	assert( pa != Id() );

	Id plot = shell_->doCreate( "Table", pa, tail, 1 );
	assert( plot != Id() );

	temp = graph + "/" + tail;
	plotIds_[ temp ] = plot; 

	numPlot_++;
	return plot;
}

enum GenesisTableModes {TAB_IO, TAB_LOOP, TAB_ONCE, TAB_BUF, TAB_SPIKE,
	TAB_FIELDS, TAB_DELAY };

Id ReadKkit::buildTable( const vector< string >& args )
{
	string head;
	string clean = cleanPath( args[2] );
	string tail = pathTail( clean, head ); // Name of xtab

	Id pa = shell_->doFind( head ).id;
	assert( pa != Id() );
	Id tab;

	int mode = atoi( args[ tableMap_[ "step_mode" ] ].c_str() );
	if ( mode == TAB_IO ) {
	} else if ( mode == TAB_LOOP || mode == TAB_ONCE ) {
		tab = shell_->doCreate( "StimulusTable", pa, tail, 1 );
		assert( tab != Id() );
		double stepSize = atof( args[ tableMap_[ "stepsize" ] ].c_str() );
		Field< double >::set( tab, "stepSize", stepSize );
		if ( mode == TAB_LOOP )
			Field< bool >::set( tab, "doLoop", 1 );
		double input = atof( args[ tableMap_[ "input" ] ].c_str() );
		Field< double >::set( tab, "startTime", -input );
		// The other StimulusTable parameters will have to wait till the
		// loadTab is invoked.
	}


	string temp = clean.substr( 10 );
	tabIds_[ temp ] = tab; 

	Id info = buildInfo( tab, tableMap_, args );

	return tab;
}

unsigned int ReadKkit::loadTab( const vector< string >& args )
{
	Id tab;
	unsigned int start = 0;
	if ( args[1].substr( 0, 5 ) == "-cont" || args[1] == "-end" ) {
		start = 2;
		tab = lastTab_;
		assert( tab != Id() );
	} else {
		tabEntries_.resize( 0 );
		start = 7;
		assert( args.size() >= start );
		lastTab_ = tab = Id( basePath_ + args[1] );
		assert( tab != Id() );
		// int calc_mode = atoi( args[3].c_str() );
		// int xdivs = atoi( args[4].c_str() );
		if ( tab.element()->cinfo()->isA( "StimulusTable" ) ) {
			double xmin = atof( args[5].c_str() );
			double xmax = atof( args[6].c_str() );
			double start = Field< double >::get( tab, "startTime" );
			start += xmin;
			Field< double >::set( tab, "startTime", start );
			Field< double >::set( tab, "stopTime", xmax );
		}
	}

	for ( unsigned int i = start; i < args.size(); ++i ) {
		tabEntries_.push_back( atof( args[i].c_str() ) );
	}
	bool ok = Field< vector< double > >::set( tab, "vector", tabEntries_ );
	assert( ok );

	// cout << "Loading table for " << args[0] << "," << args[1] << "," << clean << endl;
	
	if ( args[1] == "-end" )
		lastTab_ = Id();

	return 0;
}

void ReadKkit::innerAddMsg( 
	const string& src, const map< string, Id >& m1, const string& srcMsg,
	const string& dest, const map< string, Id >& m2, const string& destMsg,
	bool isBackward )
{
	map< string, Id >::const_iterator i = m1.find( src );
	assert( i != m1.end() );
	Id srcId = i->second;

	i = m2.find( dest );
	assert( i != m2.end() );
	Id destId = i->second;

	// dest pool is substrate of src reac
	if ( isBackward ) {
		ObjId ret = shell_->doAddMsg( "AllToOne", 
			ObjId( srcId, 0 ), srcMsg,
			ObjId( destId, 0 ), destMsg ); 
		assert( ret != ObjId() );
	} else {
		ObjId ret = shell_->doAddMsg( "OneToAll", 
			ObjId( srcId, 0 ), srcMsg,
			ObjId( destId, 0 ), destMsg ); 
		assert( ret != ObjId() );
	}
}


void ReadKkit::addmsg( const vector< string >& args)
{
	string src = cleanPath( args[1] ).substr( 10 );
	string dest = cleanPath( args[2] ).substr( 10 );
	
	if ( args[3] == "REAC" ) {
		if ( args[4] == "A" && args[5] == "B" ) {
			// Ignore kchans
			if ( chanIds_.find( src ) != chanIds_.end() )
				; // found a kchan, do nothing
			else
				innerAddMsg( src, reacIds_, "sub", dest, poolIds_, "reac");
		} 
		else if ( args[4] == "B" && args[5] == "A" ) {
			// Ignore kchans
			if ( chanIds_.find( src ) != chanIds_.end() )
				; // found a kchan, do nothing
			else
				// dest pool is product of src reac
				innerAddMsg( src, reacIds_, "prd", dest, poolIds_, "reac");
		}
		else if ( args[4] == "sA" && args[5] == "B" ) {
			// Msg from enzyme to substrate.
			if ( mmEnzIds_.find( src ) == mmEnzIds_.end() )
				innerAddMsg( src, enzIds_, "sub", dest, poolIds_, "reac" );
			else
				innerAddMsg( src, mmEnzIds_, "sub", dest, poolIds_, "reac" );
		}
	}
	else if ( args[3] == "ENZYME" ) { // Msg from enz pool to enz site
		if ( mmEnzIds_.find( dest ) == mmEnzIds_.end() )
			innerAddMsg( dest, enzIds_, "enz", src, poolIds_, "reac" );
		else
			innerAddMsg( src, poolIds_, "nOut", dest, mmEnzIds_, "enzDest", 1);
			// innerAddMsg( dest, mmEnzIds_, "enz", src, poolIds_, "nOut", 1);
		/*
		if ( mmEnzIds_.find( dest ) == mmEnzIds_.end() )
			innerAddMsg( src, poolIds_, "reac", dest, enzIds_, "enz" );
		else
			innerAddMsg( src, poolIds_, "nOut", dest, mmEnzIds_, "enz" );
			*/
	}
	else if ( args[3] == "MM_PRD" ) { // Msg from enz to Prd pool
		if ( mmEnzIds_.find( src ) == mmEnzIds_.end() )
			innerAddMsg( src, enzIds_, "prd", dest, poolIds_, "reac" );
		else
			innerAddMsg( src, mmEnzIds_, "prd", dest, poolIds_, "reac" );
	}
	else if ( args[3] == "PLOT" ) { // Time-course output for pool
		string head;
		string temp;
		dest = pathTail( cleanPath( args[2] ), head );
		string graph = pathTail( head, temp );
		temp = graph + "/" + dest;
		map< string, Id >::const_iterator i = plotIds_.find( temp );
		assert( i != plotIds_.end() );
		Id plot = i->second;

		i = poolIds_.find( src );
		Id pool;
		if ( i == poolIds_.end() ) {
			i = enzIds_.find( src ); // might be plotting an enzCplx.
			if ( i == enzIds_.end() ) {
				cout << "Error: ReadKkit: Unable to find src for plot: " <<
					src << ", " << dest << endl;
				assert( 0 );
				return;
			}
			vector< Id > enzcplx;
			i->second.element()->getNeighbours( enzcplx, 
				i->second.element()->cinfo()->findFinfo( "toCplx" ) );
			assert( enzcplx.size() == 1 );
			pool = enzcplx[0];
		}  else {
			pool = i->second;
		}

		if ( args[4] == "Co" || args[4] == "CoComplex" ) {
			ObjId ret = shell_->doAddMsg( "Single",
				plot, "requestOut", pool, "getConc" );
			assert( ret != ObjId() );
		} else if ( args[4] == "n" || args[4] == "nComplex") {
			ObjId ret = shell_->doAddMsg( "Single",
				plot, "requestOut", pool, "getN" );
			assert( ret != ObjId() );
		} else {
			cout << "Unknown PLOT msg field '" << args[4] << "'\n";
		}
	}
	else if ( args[3] == "SUMTOTAL" ) { // Summation function.
		buildSumTotal( src, dest );
	}
	else if ( args[3] == "SLAVE" ) { // Summation function.
		if ( args[4] == "output" ) {
			setupSlaveMsg( src, dest );
		}
	}
}

void ReadKkit::setupSlaveMsg( const string& src, const string& dest )
{
	// Convert the pool to a BufPool, if it isn't one already
	Id destId( basePath_ + "/kinetics/" + dest );
	assert( destId != Id() );

	if( !destId.element()->cinfo()->isA( "BufPool" )) {
		destId.element()->zombieSwap( BufPool::initCinfo() );
	}

	map< string, Id >* nameMap;
	// Check if the src is a table or a stim
	Id srcId( basePath_ + "/kinetics/" + src );
	assert( srcId != Id() );
	string output = "output";
	if ( srcId.element()->cinfo()->isA( "TableBase" ) ) {
		nameMap = &tabIds_;
	} else if ( srcId.element()->cinfo()->isA( "PulseGen" ) ) {
		nameMap = &stimIds_;
		output = "output";
	} else {
		cout << "Error: Unknown source for SLAVE msg: (" << src << 
			", " << dest << ")\n";
		return;
	}

	// NSLAVE is 1, CONCSLAVE is 2.
	map< Id, int >::iterator i = poolFlags_.find( destId );
	if ( i == poolFlags_.end() || !( i->second & 2 ) ) {
		innerAddMsg( src, *nameMap, output, dest, poolIds_, 
			"setNInit" );
	} else {
		innerAddMsg( src, *nameMap, output, dest, poolIds_,
			"setConcInit" );

		double CONCSCALE = 0.001;
		// Rescale from uM to millimolar.
		if ( nameMap == &tabIds_ ) {
			SetGet2< double, double >::set( srcId, "linearTransform",
				CONCSCALE, 0 );
		} else if ( nameMap == &stimIds_ ) {
			double x = Field< double >::get( srcId, "baseLevel" );
			Field< double >::set( srcId, "baseLevel", x * CONCSCALE );
			x = Field< double >::get( srcId, "firstLevel" );
			Field< double >::set( srcId, "firstLevel", x * CONCSCALE );
			x = Field< double >::get( srcId, "secondLevel" );
			Field< double >::set( srcId, "secondLevel", x * CONCSCALE );
		}
	}
	// cout << "Added slave msg from " << src << " to " << dest << endl;
}

// We have a slight problem because MOOSE has a more precise value for
// NA than does kkit. Also, at the time the model is loaded, the volume
// relationships are unknown. So we need to fix up conc units of all reacs.
// Here we assume that the conc units from Kkit are
// meant to be OK, so they override the #/cell (lower case k) units.
// So we convert all the Kfs and Kbs in the entire system after
// the model has been created, once we know the order of each reac.
void ReadKkit::convertParametersToConcUnits()
{
	convertPoolAmountToConcUnits();
	convertReacRatesToConcUnits();
	convertMMenzRatesToConcUnits();
	convertEnzRatesToConcUnits();
}

void ReadKkit::convertPoolAmountToConcUnits()
{
	const double NA_RATIO = KKIT_NA / NA;
	for ( map< string, Id >::iterator i = poolIds_.begin(); 
		i != poolIds_.end(); ++i ) {
		Id pool = i->second;
		double nInit = Field< double >::get( pool, "nInit" );
		double n = Field< double >::get( pool, "n" );

		nInit /= NA_RATIO;
		n /= NA_RATIO;
		Field< double >::set( pool, "nInit", nInit );
		Field< double >::set( pool, "n", n );
	}
}

void ReadKkit::convertReacRatesToConcUnits()
{
	const double NA_RATIO = KKIT_NA / NA;
	for ( map< string, Id >::iterator i = reacIds_.begin(); 
		i != reacIds_.end(); ++i ) {
		Id reac = i->second;
		double kf = Field< double >::get( reac, "Kf" );
		double kb = Field< double >::get( reac, "Kb" );
		// Note funny access here, using the Conc unit term (Kf) to get the
		// num unit term (kf). When reading, there are no volumes so the
		// Kf gets assigned to what was the kf value from kkit.

		// At this point the kf and kb are off because the
		// NA for kkit is not accurate. So we correct for this.
		unsigned int numSub = 
			Field< unsigned int >::get( reac, "numSubstrates" );
		unsigned int numPrd = 
			Field< unsigned int >::get( reac, "numProducts" );

		if ( numSub > 1 )
			kf *= pow( NA_RATIO, numSub - 1.0 );

		if ( numPrd > 1 )
			kb *= pow( NA_RATIO, numPrd - 1.0 );

		// Now we have the correct numKf and numKb, plug them into the 
		// reac, and let it internally fix up the Kf and Kb.
		Field< double >::set( reac, "numKf", kf );
		Field< double >::set( reac, "numKb", kb );
	}
}

void ReadKkit::convertMMenzRatesToConcUnits()
{
	const double NA_RATIO = KKIT_NA / NA;
	for ( map< string, Id >::iterator i = mmEnzIds_.begin(); 
		i != mmEnzIds_.end(); ++i ) {
		Id enz = i->second;
		// This was set in the original # units.
		double numKm = Field< double >::get( enz, "Km" );
		// At this point the numKm is inaaccurate because the
		// NA for kkit is not accurate. So we correct for this.
		double numSub = 
			Field< unsigned int >::get( enz, "numSubstrates" );
		// Note that we always have the enz itself as a substrate term.
		if ( numSub > 0 ) 
			numKm *= pow( NA_RATIO, -numSub );

		// Now we have the correct numKm, plug it into the MMenz, and
		// let it internally fix up the Km
		Field< double >::set( enz, "numKm", numKm );
	}
}

// we take k2 and k3 as correct, since those are just time^-1.
// Here we just need to convert k1. Originally values were set as
// k1, k2, k3 from the kkit file. k1 will need to change a bit because
// of the NA and KKIT_NA discrepancy.
void ReadKkit::convertEnzRatesToConcUnits()
{
	const double NA_RATIO = KKIT_NA / NA;
	for ( map< string, Id >::iterator i = enzIds_.begin(); 
		i != enzIds_.end(); ++i ) {
		Id enz = i->second;
		double k1 = Field< double >::get( enz, "k1" );
		// At this point the k1 is inaaccurate because the
		// NA for kkit is not accurate. So we correct for this.
		double numSub = 
			Field< unsigned int >::get( enz, "numSubstrates" );
		// Note that we always have the enz itself as a substrate term.
		if ( numSub > 0 ) 
			k1 *= pow( NA_RATIO, numSub );
		Field< double >::set( enz, "k1", k1 );
	}
}
