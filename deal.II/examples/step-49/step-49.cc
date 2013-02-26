/* $Id$
 *
 * Copyright (C) 2013 by the deal.II authors
 *
 * This file is subject to QPL and may not be  distributed
 * without copyright and license information. Please refer
 * to the file deal.II/doc/license.html for the  text  and
 * further information on this license.
 */


// This tutorial program is odd in the sense that, unlike for most other
// steps, the introduction already provides most of the information on how to
// use the various strategies to generate meshes. Consequently, there is
// little that remains to be commented on here, and we intersperse the code
// with relatively little text. In essence, the code here simply provides a
// reference implementation of what has already been described in the
// introduction.

// @sect3{Include files}

#include <deal.II/grid/tria.h>
#include <deal.II/grid/tria_accessor.h>
#include <deal.II/grid/tria_iterator.h>
#include <deal.II/grid/grid_generator.h>
#include <deal.II/grid/grid_tools.h>
#include <deal.II/grid/tria_boundary_lib.h>
#include <deal.II/grid/grid_out.h>
#include <deal.II/grid/grid_in.h>

#include <fstream>

#include <map>

using namespace dealii;

// @sect3{Generating output for a given mesh}

// The following function generates some output for any of the meshes we will
// be generating in the remainder of this program. In particular, it generates
// the following information:
//
// - Some general information about the number of space dimensions in which
//   this mesh lives and its number of cells.
// - The number of boundary faces that use each boundary indicator, so that
//   it can be compared with what we expect.
//
// Finally, the function outputs the mesh in encapsulated postscript (EPS)
// format that can easily be visualized in the same way as was done in step-1.
template<int dim>
void mesh_info(const Triangulation<dim> &tria,
	       const std::string        &filename)
{
  std::cout << "Mesh info:" << std::endl
            << " dimension: " << dim << std::endl
            << " no. of cells: " << tria.n_active_cells() << std::endl;

  // Next loop over all faces of all cells and find how often each boundary
  // indicator is used:
  {
    std::map<unsigned int, unsigned int> boundary_count;
    typename Triangulation<dim>::active_cell_iterator
    cell = tria.begin_active(),
    endc = tria.end();
    for (; cell!=endc; ++cell)
      {
        for (unsigned int face=0; face<GeometryInfo<dim>::faces_per_cell; ++face)
          {
            if (cell->face(face)->at_boundary())
              boundary_count[cell->face(face)->boundary_indicator()]++;
          }
      }

    std::cout << " boundary indicators: ";
    for (std::map<unsigned int, unsigned int>::iterator it=boundary_count.begin();
         it!=boundary_count.end();
         ++it)
      {
        std::cout << it->first << "(" << it->second << " times) ";
      }
    std::cout << std::endl;
  }

  // Finally, produce a graphical representation of the mesh to an output
  // file:
  std::ofstream out (filename.c_str());
  GridOut grid_out;
  grid_out.write_eps (tria, out);
  std::cout << " written to " << filename
	    << std::endl
	    << std::endl;
}

// @sect3{Main routines}

// @sect4{grid_1: Loading a mesh generated by gmsh}

// In this first example, we show how to load the mesh for which we have
// discussed in the introduction how to generate it. This follows the same
// pattern as used in step-5 to load a mesh, although there it was written in
// a different file format (UCD instead of MSH).
void grid_1 ()
{
  Triangulation<2> triangulation;

  GridIn<2> gridin;
  gridin.attach_triangulation(triangulation);
  std::ifstream f("untitled.msh");
  gridin.read_msh(f);

  mesh_info(triangulation, "grid-1.eps");

}


// @sect4{grid_2: Merging triangulations}

// Here, we first create two triangulations and then merge them into one.  As
// discussed in the introduction, it is important to ensure that the vertices
// at the common interface are located at the same coordinates.
void grid_2 ()
{
  Triangulation<2> tria1;
  GridGenerator::hyper_cube_with_cylindrical_hole (tria1, 0.25, 1.0);

  Triangulation<2> tria2;
  std::vector< unsigned int > repetitions(2);
  repetitions[0]=3;
  repetitions[1]=2;
  GridGenerator::subdivided_hyper_rectangle (tria2, repetitions,
                                             Point<2>(1.0,-1.0),
					     Point<2>(4.0,1.0));

  Triangulation<2> triangulation;
  GridGenerator::merge_triangulations (tria1, tria2, triangulation);

  mesh_info(triangulation, "grid-2.eps");
}


// @sect4{grid_3: Moving vertices}

// In this function, we move vertices of a mesh. This is simpler than one
// usually expects: if you ask a cell using <code>cell-@>vertex(i)</code> for
// the coordinates of its <code>i</code>th vertex, it doesn't just provide the
// location of this vertex but in fact a reference to the location where these
// coordinates are stored. We can then modify the value stored there.
//
// So this is what we do in the first part of this function: We create a
// square of geometry $[-1,1]^2$ with a circular hole with radius 0.25 located
// at the origin. We then loop over all cells and all vertices and if a vertex
// has a $y$ coordinate equal to one, we move it upward by 0.5.
//
// Note that this sort of procedure does not usually work this way because one
// will typically encounter the same vertices multiple times and may move them
// more than once. It works here because we select the vertices we want to use
// based on their geometric location, and a vertex moved once will fail this
// test in the future. A more general approach to this problem would have been
// to keep a std::set of of those vertex indices that we have already moved
// (which we can obtain using <code>cell-@>vertex_index(i)</code> and only
// move those vertices whose index isn't in the set yet.
void grid_3 ()
{
  Triangulation<2> triangulation;
  GridGenerator::hyper_cube_with_cylindrical_hole (triangulation, 0.25, 1.0);

  Triangulation<2>::active_cell_iterator
  cell = triangulation.begin_active(),
  endc = triangulation.end();
  for (; cell!=endc; ++cell)
    {
      for (unsigned int i=0; i<GeometryInfo<2>::vertices_per_cell; ++i)
        {
          Point<2> &v = cell->vertex(i);
          if (std::abs(v(1)-1.0)<1e-5)
            v(1) += 0.5;
        }
    }

  // In the second step we will refine the mesh twice. To do this correctly,
  // we have to associate a geometry object with the boundary of the hole;
  // since the boundary of the hole has boundary indicator 1 (see the
  // documentation of the function that generates the mesh), we need to create
  // an object that describes a circle (i.e., a hyper ball) with appropriate
  // center and radius and assign it to the triangulation. We can then refine
  // twice:
  const HyperBallBoundary<2> boundary_description(Point<2>(0,0), 0.25);
  triangulation.set_boundary (1, boundary_description);
  triangulation.refine_global(2);

  // The mesh so generated is then passed to the function that generates
  // output. In a final step we remove the boundary object again so that it is
  // no longer in use by the triangulation when it is destroyed (the boundary
  // object is destroyed first in this function since it was declared after
  // the triangulation).
  mesh_info (triangulation, "grid-3.eps");
  triangulation.set_boundary (1);
}


// @sect4{grid_4: Demonstrating extrude_triangulation}

// This example takes the initial grid from the previous function and simply extrudes it into the third space dimension:
void grid_4()
{
  Triangulation<2> triangulation;
  Triangulation<3> out;
  GridGenerator::hyper_cube_with_cylindrical_hole (triangulation, 0.25, 1.0);

  GridGenerator::extrude_triangulation (triangulation, 3, 2.0, out);
  mesh_info(out, "grid-4.eps");

}


// @sect4{grid_5: Demonstrating GridTools::transform, part 1}

// This and the next example first create a mesh and then transform it by
// moving every node of the mesh according to a function that takes a point
// and returns a mapped point. In this case, we transform $(x,y) \mapsto
// (x,y+\sin(\pi x/5))$.
//
// GridTools::transform takes a triangulation and any kind of object that can
// be called like a function as arguments. This function-like argument can be
// simply the address of a function as in the current case, or an object that
// has an <code>operator()</code> as in the next example, or for example a
// <code>std::function@<Point@<2@>(const Point@<2@>)@></code> object one can get
// via <code>std::bind</code> in more complex cases.
Point<2> grid_5_transform (const Point<2> &in)
{
  return Point<2>(in(0),
		  in(1) + std::sin(in(0)/5.0*3.14159));
}


void grid_5()
{
  Triangulation<2> tria;
  std::vector<unsigned int> repetitions(2);
  repetitions[0] = 14;
  repetitions[1] = 2;
  GridGenerator::subdivided_hyper_rectangle (tria, repetitions,
                                             Point<2>(0.0,0.0),
					     Point<2>(10.0,1.0));

  GridTools::transform(&grid_5_transform, tria);
  mesh_info(tria, "grid-5.eps");
}



// @sect4{grid_6: Demonstrating GridTools::transform, part 2}

// In this second example of transforming points from an original to a new
// mesh, we will use the mapping $(x,y) \mapsto (x,\tanh(2y)/\tanh(2))$. To
// make things more interesting, rather than doing so in a single function as
// in the previous example, we here create an object with an
// <code>operator()</code> that will be called by GridTools::transform. Of
// course, this object may in reality be much more complex: the object may
// have member variables that play a role in computing the new locations of
// vertices.
struct Grid6Func
{
    double trans(const double y) const
      {
	return std::tanh(2*y)/tanh(2);
      }

    Point<2> operator() (const Point<2> & in) const
      {
	return Point<2> (in(0),
			 trans(in(1)));
      }
};


void grid_6()
{
  Triangulation<2> tria;
  std::vector< unsigned int > repetitions(2);
  repetitions[0] = repetitions[1] = 40;
  GridGenerator::subdivided_hyper_rectangle (tria, repetitions,
                                             Point<2>(0.0,0.0),
					     Point<2>(1.0,1.0));

  GridTools::transform(Grid6Func(), tria);
  mesh_info(tria, "grid-6.eps");
}


// @sect4{grid_7: Demonstrating distort_random}

// In this last example, we create a mesh and then distort its (interior)
// vertices by a random perturbation. This is not something you want to do for
// production computations, but it is a useful tool for testing
// discretizations and codes to make sure they don't work just by accident
// because the mesh happens to be uniformly structured and supporting
// super-convergence properties.
void grid_7()
{
  Triangulation<2> tria;
  std::vector<unsigned int> repetitions(2);
  repetitions[0] = repetitions[1] = 16;
  GridGenerator::subdivided_hyper_rectangle (tria, repetitions,
                                             Point<2>(0.0,0.0),
					     Point<2>(1.0,1.0));

  GridTools::distort_random (0.3, tria, true);
  mesh_info(tria, "grid-7.eps");
}


// @sect3{The main function}

// Finally, the main function. There isn't much to do here, only to call the
// subfunctions.
int main ()
{
  grid_1 ();
  grid_2 ();
  grid_3 ();
  grid_4 ();
  grid_5 ();
  grid_6 ();
  grid_7 ();
}
