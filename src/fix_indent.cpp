/* ----------------------------------------------------------------------
 *
 *                    ***       Karamelo       ***
 *               Parallel Material Point Method Simulator
 * 
 * Copyright (2019) Alban de Vaucorbeil, alban.devaucorbeil@monash.edu
 * Materials Science and Engineering, Monash University
 * Clayton VIC 3800, Australia

 * This software is distributed under the GNU General Public License.
 *
 * ----------------------------------------------------------------------- */

#include <iostream>
#include <vector>
#include <string>
#include <Eigen/Eigen>
#include "fix_indent.h"
#include "input.h"
#include "group.h"
#include "domain.h"
#include "input.h"
#include "universe.h"
#include "update.h"
#include "solid.h"
#include "error.h"

using namespace std;
using namespace FixConst;
using namespace Eigen;

FixIndent::FixIndent(MPM *mpm, vector<string> args) : Fix(mpm, args)
{
  if (args.size() < 9) {
    error->all(FLERR,"Error: too few arguments for fix_body_force_current_config: requires at least 9 arguments. " + to_string(args.size()) + " received.\n");
  }

  if (group->pon[igroup].compare("particles") !=0 && group->pon[igroup].compare("all") !=0) {
    error->all(FLERR, "fix_indent needs to be given a group of nodes" + group->pon[igroup] + ", " + args[2] + " is a group of " + group->pon[igroup] + ".\n");
  }
  cout << "Creating new fix FixIndent with ID: " << args[0] << endl;
  id = args[0];

  type = args[3];
  if (args[3].compare("sphere")==0) {
    type = "sphere";
  } else {
    error->all(FLERR,"Error indent type " + args[3] + " unknown. Only type sphere is supported.\n");
  }

  Kpos = 4;
  Rpos = 5;
  xpos = 6;
  ypos = 7;
  zpos = 8;
}

FixIndent::~FixIndent()
{
}

void FixIndent::init()
{
}

void FixIndent::setup()
{
}

void FixIndent::setmask() {
  mask = 0;
  mask |= INITIAL_INTEGRATE;
}


void FixIndent::initial_integrate() {
  // cout << "In FixIndent::initial_integrate()\n";

  // Go through all the particles in the group and set b to the right value:
  Eigen::Vector3d f;

  int solid = group->solid[igroup];

  Solid *s;
  Eigen::Vector3d ftot, ftot_reduced;

  double K = input->parsev(args[Kpos]).result(mpm);
  double R = input->parsev(args[Rpos]).result(mpm);
  Eigen::Vector3d xs(input->parsev(args[xpos]).result(mpm),
		     input->parsev(args[ypos]).result(mpm),
		     input->parsev(args[zpos]).result(mpm));
  Eigen::Vector3d xsp, n, ftemp;

  double r, dr, fmag, fsigma, fs;

  ftot.setZero();
  fs = 0;

  bool tl;
  if (update->method_type.compare("tlmpm") == 0 ||
      update->method_type.compare("tlcpdi") == 0)
    tl = true;
  else
    tl = false;

  if (solid == -1) {
    for (int isolid = 0; isolid < domain->solids.size(); isolid++) {
      s = domain->solids[isolid];

      for (int ip = 0; ip < s->np_local; ip++) {
	if (s->mass[ip] > 0) {
	  if (s->mask[ip] & groupbit) {
	    // Gross screening:
	    xsp = s->x[ip] - xs;

	    if (( xsp[0] < R ) && ( xsp[1] < R ) && ( xsp[2] < R )
		&& ( xsp[0] > -R ) && ( xsp[1] > -R ) && ( xsp[2] > -R )) {

	      r = xsp.norm();
	      // Finer screening:
	      if (r < R) {
		dr = r - R;
		fmag = K*dr*dr;
		// Maybe fmag should be inversely proportional to the mass of the particle!!
		n = xsp / r;
		f = fmag * n;
		s->mbp[ip] += f;
		ftot += f;
		if (tl)
		  ftemp = (s->R[ip] * s->sigma[ip] * s->R[ip].transpose()) * n;
		else
		  ftemp = s->sigma[ip] * n;
		fs += ftemp[1] * pow(s->vol[ip], 2./3.);
	      }
	    }
	  }
	}
      }
    }
  } else {
    s = domain->solids[solid];

    for (int ip = 0; ip < s->np_local; ip++) {
      if (s->mass[ip] > 0) {
	if (s->mask[ip] & groupbit) {
	  // Gross screening:
	  xsp = s->x[ip] - xs;
	  if (( xsp[0] < R ) && ( xsp[1] < R ) && ( xsp[2] < R )
	      && ( xsp[0] > -R ) && ( xsp[1] > -R ) && ( xsp[2] > -R )) {

	    r = xsp.norm();
	    // Finer screening:
	    if (r < R) {
	      dr = r - R;
	      fmag = K*dr*dr;
	      // Maybe fmag should be inversely proportional to the mass of the particle!!
	      n = xsp / r;
	      f = fmag * n;
	      s->mbp[ip] += f;
	      ftot += f;
	      if (tl)
		ftemp = (s->R[ip] * s->sigma[ip] * s->R[ip].transpose()) * n;
	      else
		ftemp = s->sigma[ip] * n;
	      fs += ftemp[1] * pow(s->vol[ip], 2./3.);
	    }
	  }
	}
      }
    }
  }

  // Reduce ftot:
  double fs_reduced;
  MPI_Allreduce(&fs,&fs_reduced,1,MPI_DOUBLE,MPI_SUM,universe->uworld);
  MPI_Allreduce(ftot.data(),ftot_reduced.data(),3,MPI_DOUBLE,MPI_SUM,universe->uworld);

  (*input->vars)[id+"_s"]=Var(id+"_s", fs_reduced);
  (*input->vars)[id+"_x"]=Var(id+"_x", ftot_reduced[0]);
  (*input->vars)[id+"_y"]=Var(id+"_y", ftot_reduced[1]);
  (*input->vars)[id+"_z"]=Var(id+"_z", ftot_reduced[2]);
  // cout << "f for " << n << " nodes from solid " << domain->solids[isolid]->id << " set." << endl;
  // cout << "ftot = [" << ftot[0] << ", " << ftot[1] << ", " << ftot[2] << "]\n"; 
}
