#include "mpm.h"
#include "grid.h"
#include "material.h"
#include "input.h"
#include <vector>
#include "memory.h"
#include "update.h"

using namespace std;


Grid::Grid(MPM *mpm) :
  Pointers(mpm)
{
  cout << "Creating new grid" << endl;

  x= NULL;
  v = v_update = NULL;
  b = f = NULL;

  mass = NULL;
}

Grid::~Grid()
{
  memory->destroy(x);
  memory->destroy(v);
  memory->destroy(v_update);
  memory->destroy(b);
  memory->destroy(f);
  memory->destroy(mass);
}

void Grid::init(double *solidlo, double *solidhi){
  int nx = (int) (solidhi[0]-solidlo[0])/cellsize + 2;
  int ny = (int) (solidhi[1]-solidlo[1])/cellsize + 2;
  int nz = (int) (solidhi[2]-solidlo[2])/cellsize + 2;

  int nn = nx*ny*nz;
  grow(nn);


  int l=0;
  for (int i=0; i<nx; i++){
    for (int j=0; j<ny; j++){
      for (int k=0; k<nz; k++){
	x[l][0] = solidlo[0] + cellsize*i;
	x[l][1] = solidlo[1] + cellsize*j;
	x[l][2] = solidlo[2] + cellsize*k;

	v[l][0] = v[l][1] = v[l][2] = 0;
	l++;
      }
    }
  }
}

void Grid::setup(string cs){
  cellsize = input->parse(cs);
  cout << "Set grid cellsize to " << cellsize << endl;
}

void Grid::grow(int nn){
  nnodes = nn;

  if (x == NULL) x = new Eigen::Vector3d[nn];
  else {
    cout << "Error: x already exists, I don't know how to grow it!\n";
    exit(1);
  }

  if (v == NULL) v = new Eigen::Vector3d[nn];
  else {
    cout << "Error: v already exists, I don't know how to grow it!\n";
    exit(1);
  }

  if (v_update == NULL) v_update = new Eigen::Vector3d[nn];
  else {
    cout << "Error: v_update already exists, I don't know how to grow it!\n";
    exit(1);
  }

  if (b == NULL) b = new Eigen::Vector3d[nn];
  else {
    cout << "Error: b already exists, I don't know how to grow it!\n";
    exit(1);
  }

  if (f == NULL) f = new Eigen::Vector3d[nn];
  else {
    cout << "Error: f already exists, I don't know how to grow it!\n";
    exit(1);
  }

  string str = "grid-mass";
  cout << "Growing " << str << endl;
  mass = memory->grow(mass, nn, str);
}

void Grid::update_grid_velocities()
{
  for (int i=0; i<nnodes; i++){
    v_update[i] = v[i];
    if (mass[i] > 0) v_update[i] += update->dt/mass[i] * (f[i] + b[i]);
  }
}
