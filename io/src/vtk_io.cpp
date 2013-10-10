/*
 * Software License Agreement (BSD License)
 *
 *  Point Cloud Library (PCL) - www.pointclouds.org
 *  Copyright (c) 2010-2011, Willow Garage, Inc.
 *
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above
 *     copyright notice, this list of conditions and the following
 *     disclaimer in the documentation and/or other materials provided
 *     with the distribution.
 *   * Neither the name of Willow Garage, Inc. nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 *  FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 *  COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 *  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 *  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 *  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 *  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 *  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 *  ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 *
 * $Id$
 *
 */

#include <pcl/point_types.h>
#include <pcl/io/vtk_io.h>
#include <fstream>
#include <iostream>
#include <pcl/common/io.h>

//////////////////////////////////////////////////////////////////////////////////////////////
int
pcl::io::saveVTKFile (const std::string &file_name,
                      const pcl::PolygonMesh &triangles, unsigned precision)
{
  if (triangles.cloud.data.empty ())
  {
    PCL_ERROR ("[pcl::io::saveVTKFile] Input point cloud has no data!\n");
    return (-1);
  }

  // Open file
  std::ofstream fs;
  fs.precision (precision);
  fs.open (file_name.c_str ());

  unsigned int nr_points  = triangles.cloud.width * triangles.cloud.height;
  unsigned int point_size = static_cast<unsigned int> (triangles.cloud.data.size () / nr_points);

  // Write the header information
  fs << "# vtk DataFile Version 3.0\nvtk output\nASCII\nDATASET POLYDATA\nPOINTS " << nr_points << " float" << std::endl;

  // Iterate through the points
  for (unsigned int i = 0; i < nr_points; ++i)
  {
    int xyz = 0;
    for (size_t d = 0; d < triangles.cloud.fields.size (); ++d)
    {
      int count = triangles.cloud.fields[d].count;
      if (count == 0)
        count = 1;          // we simply cannot tolerate 0 counts (coming from older converter code)
      int c = 0;
      if ((triangles.cloud.fields[d].datatype == pcl::PCLPointField::FLOAT32) && (
           triangles.cloud.fields[d].name == "x" ||
           triangles.cloud.fields[d].name == "y" ||
           triangles.cloud.fields[d].name == "z"))
      {
        float value;
        memcpy (&value, &triangles.cloud.data[i * point_size + triangles.cloud.fields[d].offset + c * sizeof (float)], sizeof (float));
        fs << value;
        if (++xyz == 3)
          break;
      }
      fs << " ";
    }
    if (xyz != 3)
    {
      PCL_ERROR ("[pcl::io::saveVTKFile] Input point cloud has no XYZ data!\n");
      return (-2);
    }
    fs << std::endl;
  }

  // Write vertices
  fs << "\nVERTICES " << nr_points << " " << 2*nr_points << std::endl;
  for (unsigned int i = 0; i < nr_points; ++i)
    fs << "1 " << i << std::endl;

  // Write polygons
  // compute the correct number of values:
  size_t triangle_size = triangles.polygons.size ();
  size_t correct_number = triangle_size;
  for (size_t i = 0; i < triangle_size; ++i)
    correct_number += triangles.polygons[i].vertices.size ();
  fs << "\nPOLYGONS " << triangle_size << " " << correct_number << std::endl;
  for (size_t i = 0; i < triangle_size; ++i)
  {
    fs << triangles.polygons[i].vertices.size () << " ";
    size_t j = 0;
    for (j = 0; j < triangles.polygons[i].vertices.size () - 1; ++j)
      fs << triangles.polygons[i].vertices[j] << " ";
    fs << triangles.polygons[i].vertices[j] << std::endl;
  }

  // Write RGB values
  int field_index = getFieldIndex (triangles.cloud, "rgb");
  if (field_index != -1)
  {
    fs << "\nPOINT_DATA " << nr_points << "\nCOLOR_SCALARS scalars 3\n";
    for (unsigned int i = 0; i < nr_points; ++i)
    {
      int count = triangles.cloud.fields[field_index].count;
      if (count == 0)
        count = 1;          // we simply cannot tolerate 0 counts (coming from older converter code)
      int c = 0;
      if (triangles.cloud.fields[field_index].datatype == pcl::PCLPointField::FLOAT32)
      {
        pcl::RGB color;
        memcpy (&color, &triangles.cloud.data[i * point_size + triangles.cloud.fields[field_index].offset + c * sizeof (float)], sizeof (pcl::RGB));
        int r = color.r;
        int g = color.g;
        int b = color.b;
        fs << static_cast<float> (r) / 255.0f << " " << static_cast<float> (g) / 255.0f << " " << static_cast<float> (b) / 255.0f;
      }
      fs << std::endl;
    }
  }

  // Close file
  fs.close ();
  return (0);
}

//////////////////////////////////////////////////////////////////////////////////////////////
int
pcl::io::saveVTKFile (const std::string &file_name,
                      const pcl::PCLPointCloud2 &cloud, unsigned precision)
{
  if (cloud.data.empty ())
  {
    PCL_ERROR ("[pcl::io::saveVTKFile] Input point cloud has no data!\n");
    return (-1);
  }

  // Open file
  std::ofstream fs;
  fs.precision (precision);
  fs.open (file_name.c_str ());

  unsigned int nr_points  = cloud.width * cloud.height;
  unsigned int point_size = static_cast<unsigned int> (cloud.data.size () / nr_points);

  // Write the header information
  fs << "# vtk DataFile Version 3.0\nvtk output\nASCII\nDATASET POLYDATA\nPOINTS " << nr_points << " float" << std::endl;

  // Iterate through the points
  for (unsigned int i = 0; i < nr_points; ++i)
  {
    int xyz = 0;
    for (size_t d = 0; d < cloud.fields.size (); ++d)
    {
      int count = cloud.fields[d].count;
      if (count == 0)
        count = 1;          // we simply cannot tolerate 0 counts (coming from older converter code)
      int c = 0;
      if ((cloud.fields[d].datatype == pcl::PCLPointField::FLOAT32) && (
           cloud.fields[d].name == "x" ||
           cloud.fields[d].name == "y" ||
           cloud.fields[d].name == "z"))
      {
        float value;
        memcpy (&value, &cloud.data[i * point_size + cloud.fields[d].offset + c * sizeof (float)], sizeof (float));
        fs << value;
        if (++xyz == 3)
          break;
      }
      fs << " ";
    }
    if (xyz != 3)
    {
      PCL_ERROR ("[pcl::io::saveVTKFile] Input point cloud has no XYZ data!\n");
      return (-2);
    }
    fs << std::endl;
  }

  // Write vertices
  fs << "\nVERTICES " << nr_points << " " << 2*nr_points << std::endl;
  for (unsigned int i = 0; i < nr_points; ++i)
      fs << "1 " << i << std::endl;

  /**
   * The following variable (PointDataPrinted) checks if the line 'POINT_DATA
   * NumPoints' has already been written or not. As this line should only be
   * written once according to VTK Legacy File Format, when there are multiple
   * Fields (like COLOR_SCALARS, SCALARS, NORMALS, etc.) to be written.
   */
  bool PointDataPrinted = false;

  // Write RGB values
  int field_index = getFieldIndex (cloud, "rgb");
  if (field_index != -1)
  {
      if (!PointDataPrinted)
      {
          fs << "\nPOINT_DATA " << nr_points;
          PointDataPrinted = true;
      }
      fs << "\nCOLOR_SCALARS scalars 3\n";
      for (unsigned int i = 0; i < nr_points; ++i)
      {
          int count = cloud.fields[field_index].count;
          if (count == 0)
              count = 1;          // we simply cannot tolerate 0 counts (coming from older converter code)
          int c = 0;
          if (cloud.fields[field_index].datatype == pcl::PCLPointField::FLOAT32)
          {
              pcl::RGB color;
              memcpy (&color, &cloud.data[i * point_size + cloud.fields[field_index].offset + c * sizeof (float)], sizeof (pcl::RGB));
              int r = color.r;
              int g = color.g;
              int b = color.b;
              fs << static_cast<float> (r) / 255.0f << " " << static_cast<float> (g) / 255.0f << " " << static_cast<float> (b) / 255.0f;
          }
          fs << std::endl;
      }
  }

  // Write Intensity values
  field_index = pcl::getFieldIndex (cloud, "intensity");
  if (field_index != -1)
  {
      if (!PointDataPrinted)
      {
          fs << "\nPOINT_DATA " << nr_points;
          PointDataPrinted = true;
      }
      fs << "\nSCALARS intensity_scalars float 1\nLOOKUP_TABLE my_table\n";

      for (unsigned int i = 0; i < nr_points; ++i)
      {
          int count = cloud.fields[field_index].count;
          if (count == 0)
              count = 1;          // we simply cannot tolerate 0 counts (coming from older converter code)
          int c = 0;
          if (cloud.fields[field_index].datatype == pcl::PCLPointField::FLOAT32)
          {
              float intensity;
              memcpy (&intensity, &cloud.data[i * point_size + cloud.fields[field_index].offset + c * sizeof (float)], sizeof (float));
              fs << intensity;
          }
          fs << std::endl;
      }
  }

  // Write Labels
  field_index = pcl::getFieldIndex (cloud, "label");
  if (field_index != -1)
  {
      if (!PointDataPrinted)
      {
          fs << "\nPOINT_DATA " << nr_points;
          PointDataPrinted = true;
      }
      fs << "\nSCALARS labels unsigned_int 1\nLOOKUP_TABLE label_table\n";

      for (unsigned int i = 0; i < nr_points; ++i)
      {
          int count = cloud.fields[field_index].count;
          if (count == 0)
              count = 1;          // we simply cannot tolerate 0 counts (coming from older converter code)
          int c = 0;
          if (cloud.fields[field_index].datatype == pcl::PCLPointField::UINT32)
          {
              pcl::uint32_t label;
              memcpy (&label, &cloud.data[i * point_size + cloud.fields[field_index].offset + c * sizeof (pcl::uint32_t)], sizeof (pcl::uint32_t));
              fs << label;
          }
          fs << std::endl;
      }
  }

  // Write Normals
  field_index = pcl::getFieldIndex (cloud, "normal_x");

  if (field_index != -1)
  {
      if (!PointDataPrinted)
      {
          fs << "\nPOINT_DATA " << nr_points;
          PointDataPrinted = true;
      }
      fs << "\nNORMALS point_normals float\n";

      // Iterate through the normals
      for (unsigned int i = 0; i < nr_points; ++i)
      {
          int xyz = 0;
          for (size_t d = 0; d < cloud.fields.size (); ++d)
          {
              int count = cloud.fields[d].count;
              if (count == 0)
                  count = 1;          // we simply cannot tolerate 0 counts (coming from older converter code)
              int c = 0;
              if ((cloud.fields[d].datatype == pcl::PCLPointField::FLOAT32) && (
                          cloud.fields[d].name == "normal_x" ||
                          cloud.fields[d].name == "normal_y" ||
                          cloud.fields[d].name == "normal_z"))
              {
                  float value;
                  memcpy (&value, &cloud.data[i * point_size + cloud.fields[d].offset + c * sizeof (float)], sizeof (float));
                  fs << value;
                  if (++xyz == 3)
                      break;
              }
              fs << " ";
          }
          if (xyz != 3)
          {
              PCL_ERROR ("[pcl::io::saveVTKFile] Input point cloud has no NORMAL_XYZ data!\n");
              return (-2);
          }
          fs << std::endl;
      }
  }

  // Close file
  fs.close ();
  return (0);
}

