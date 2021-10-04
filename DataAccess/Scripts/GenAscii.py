#!/tools/miniconda/bin/python
import numpy as np
import h5py
import time
import sys
import os
import re
from datetime import datetime, timedelta
#
#
# Configuration information 
#
#
import configparser
from configparser import SafeConfigParser

#
# Global variable array to hold all initial dataset names
#
var = []

intRegex = re.compile(r'int')
format = "%Y-%m-%dT%H.%M.%S.%f";
#epoch = '2018-01-01T00.00.00.000000';
#epochtime = datetime.strptime(epoch,format);

################################################################################
#
# find_datasets: Given a file an h5py file, find all of the datasets within the given
# file
#
################################################################################
def find_datasets(item):
#
# If a file or group, recursively find datasets
#
   if isinstance(item,h5py.File) or isinstance(item, h5py.Group):
      for key, val in dict(item).items():
         find_datasets(val)
#
# If a Dataset, store it in the var global variable
#
   elif isinstance(item, h5py.Dataset):
      var.append(item.name)
   return(var)
################################################################################
#
# write_blanks: pad spaces to the left of the actual data value so we line up
# the ascii output correctly.
#
################################################################################

def write_blanks(file, dataset_length, formatlength):
#  print "VAR_LENGTH: ", dataset_length 
#  print "FORMAT LENGTH: ", formatlength
#
#  The width of the column will be determined by the length of the dataset name.
#  The format length is the length of the data and the dataset_length is the length
#  of the dataset name from the h5 file. We will write blanks
#  to line up the ascii output correctly.
#
   if (formatlength < dataset_length):
      file.write('{0:1}'.format(" " * (dataset_length - formatlength + 1)))
   return
################################################################################
#
# START OF PYTHON SCRIPT
# gen_ascii.py: Python tool adapter to produce ascii data listing for GLAH
# files (tabular ascii).
#

from Logging_3 import Log

from ParameterHandling import ParameterHandling

parameters = ParameterHandling("HDF5 to tabular ASCII")
inputfile = parameters.getInputfile()
outputdir = parameters.getOutputdir()
identifier = parameters.getIdentifier()
debug = parameters.getDebug()

filepath = sys.path[0];
mode = filepath.split('/')[3];

outputdir = outputdir + "/" + identifier;
if not os.path.exists(outputdir):
    os.makedirs(outputdir);

#
# Get remove the '.H5' suffix before tacking on the .ascii suffix to form the output file
#
outputfile = outputdir + "/" + os.path.splitext(os.path.basename(inputfile))[0] + ".ascii"
file = open(outputfile, 'w')
   

log = Log(debug, outputfile)

item = h5py.File(inputfile,'r')
#
# Use find datasets function to find all bands
#
datasets=find_datasets(item)


parser = SafeConfigParser()
configfile = "/usr/ecs/" + mode + "/CUSTOM/data/DPL/GenAscii.ini";
if (debug == 'Y'):
   log.writeMessage("USING CONFIGURATION FILE: " + configfile + "\n");
parser.read(configfile);

preferred_fields = [e.strip() for e in parser.get('gen_ascii', 'preferred_fields').split(',')]
if (debug == 'Y'):
   log.writeMessage("preferred_fields: " + str(preferred_fields) + "\n");
fill_value = parser.get('gen_ascii', 'fill_value')

if (re.search(r'GLA',inputfile)):
   date_time_length = 19
else:
   date_time_length = 26

#
# We will create a new list for the final ordered variables we will print out
#
ordered_dataset_list = []

#
# Get the intersection of the datasets and the preferred fields
#
#rdered_dataset_list = list(set(datasets) & set(preferred_fields))
#f (debug == 'Y'):
#  print "Ordered_Dataset_List after intersection is: ", ordered_dataset_list

#
# Add the rest of the datasets to the ordered_dataset_list by using
# the symmetric set difference (i.e. get all of those datasets that
# were not preferred fields and add them to teh ordered_dataset_list

#ordered_dataset_list = ordered_dataset_list + list(set(datasets)^set(ordered_dataset_list))
#if (debug == 'Y'):
#   print "Ordered_Dataset_List after adding back the non-preferred fields is: ", ordered_dataset_list

   
   
#
# Loop over the configured preferred variables list and check if they are in the 
# input dataset list (datasets). If we find it, we append it to our new ordered list
# and then delete it out of the input dataset list.
#
for i in range (0, len(preferred_fields)):
   num_datasets=len(datasets)
   
   if (debug == 'Y'):
      log.writeMessage("Preferred_field: " + str(i) + ": " + preferred_fields[i] + "\n");
   for j in range (0, num_datasets):
       if (preferred_fields[i] == datasets[j]):
           ordered_dataset_list.append(datasets[j])
           del datasets[j]
           break

#
# Anything that's left in the input dataset list gets tacked onto the end of the 
# ordered list
#
for j in range (0, len(datasets)):
   ordered_dataset_list.append(datasets[j])

if (debug == 'Y'):
   log.writeMessage("ORDERED_DATASET_LIST IS: " + str(ordered_dataset_list) + "\n")

# Now our ordered list has what we want to print out
#

# Create a hash table that has the length of the dataset name
# (This will be used for formatted printing later when we print out blanks
#  for alignment purposes).
#
dataset_len_list = {}

for j in range (0, len(ordered_dataset_list)):
    dataset_len_list[ordered_dataset_list[j]] = len (ordered_dataset_list[j]) 

if (debug == 'Y'):
   log.writeMessage("DATA SET LENGTH LIST: " + str(dataset_len_list) + "\n")
   

with h5py.File(inputfile,'r') as hf:

    # Now we need to build a master ordered list from the ordered_dataset_list
    # The ordered_dataset_list contains all of the variable names in one one-dimensional
    # list however some granules may have datasets that are of different dimensions.
    # So we will go through the one-dimensional list and break out groups of variables
    # that have different dimensions. One example of Glas data is Data_40HZ groups vs. 
    # Data_1HZ groups.
    # 
    master_ordered_list = []
    #
    # Store the variable shape in the fihape[1]st element of the master list
    # The master list will be a multiple dimension list that will contain
    # the list of datasets that correspond to the same shape
    # The shape is in the first element of each list
    # 
    # Here is an example of what we would build if we were given the following
    # datasets:
    #
    #  [[(8650,), '/Data_1HZ/DS_UTCTime_1', '/Data_1HZ/Geolocation/d_lat', '/Data_1HZ/Geolocation/d_lon',
    #   [(346000,), '/Data_40HZ/Elevation_Flags/elv_gauss_flg', '/Data_40HZ/Elevation_Flags/elv_cloud_flg']]
    #
    #
    #
    # We cycle through the ordered list
    #
    for item in ordered_dataset_list[0:len(ordered_dataset_list)]:
       if (debug == 'Y'):
           log.writeMessage("ITEM IS: " + item + "\n")
       #
       # Get that shape size from the dataset item
       #
       shape_size = np.array(hf.get(item)).shape
       #
       # If the shape of the next item is different then the very first item, we will
       # place it in its own column of the master_ordered_list
       # 
       add_to_existing_list = 0
       #
       # Don't attempt to add anything higher than 2 dimensional to the list
       #
       if (len(shape_size) <= 2):
       # Now we will look over the master order list looking for the proper shape
        
          for i in range(0, len(master_ordered_list)):
             if (debug == 'Y'):
                log.writeMessage("shape_size in comparison: " + str(shape_size) + "\n")
                log.writeMessage("master_ordered_list[i][0] is: " + str(master_ordered_list[i][0]) + "\n")
             #
             # If we haven't started building the master list yet (len = 0) or
             # if we find a matching shape size in the 0th element and its not
             # multi-dimensional (len(shape_size)==1), we just append
             # the new dataset name to that position and flag that we added the new
             # element to an existing column.
             #
             # We may continue to loop over the rest of the shapes we have already 
             # stored however this shouldn't be a large list.
             #
             if ((len(master_ordered_list) == 0) or ((shape_size == master_ordered_list[i][0]))) or ((len(shape_size) == 2) and (shape_size[0] == master_ordered_list[i][0][0])):
#

                 # Create tuple with 2nd dimension of 2-dimensional datasets followed by the data set name. The following
                 # example shows that the i_rec_ndx is only 1 dimension (51440) but the second dataset has a 2nd dimension
                 # of 6.
                 # [[(51440,), (0, '/Data_40HZ/Time/i_rec_ndx'), (6, '/Data_40HZ/Waveform/d_amp2'), ....
                 #
                 if (len(shape_size) == 2):
                    dataset_tup = (shape_size[1], str(item))
                 else:
                    dataset_tup = (0, str(item))
                 master_ordered_list[i].append(dataset_tup)

                 if (debug == 'Y'):
                    log.writeMessage("Added element to existing column" + "\n")
                 add_to_existing_list = 1

          # If we have added an element to an existing column we'll print out a debug message
          if add_to_existing_list == 1:
             if (debug == 'Y'):
                log.writeMessage("Added an element to existing column" + "\n")
          else:
             if (debug == 'Y'):
                log.writeMessage("new element added!" + "\n")
             #
             # Other wise we didn't find this existing new shape. We will create
             # another list for the master ordered list and store the shape size
             # and the dataset name (in variable item)
             # 
             master_ordered_list.append([])
             master_ordered_list[len(master_ordered_list)-1].append(shape_size)

             if (len(shape_size) == 2):
                dataset_tup = (shape_size[1], str(item))
             else:
                dataset_tup = (0, str(item))
             master_ordered_list[len(master_ordered_list)-1].append(dataset_tup)  
    #
    # We have completely built our master ordered list of datasets
    #
    if (debug == 'Y'):
       log.writeMessage("LEN(MASTER_ORDERED_LIST): " + str(len(master_ordered_list)) + "\n") 
    # 
    # Now get the units of all of our datasets store it in var_unit_value
    # This will be a 3 dimensional list that contains all units values of the datasets as well
    # as any defined fill values for the particular dataset. Also included will be the 
    # datatype
    #
    # Example:
    #  VAR_UNIT_VALUE =  [[['seconds since 2000-01-01 12:00:00 UTC', '_NoFillValue', 'float64'], ['degrees_north','_NoFillValue', 'float32'], ['degrees_east', '_NoFillValue', 'float32']], [[0, -9999.0, 'float64'], [0, -9999.0, 'float64']]]
    # (Note that a value of zero indicates no units were found for the dataset and '_NoFillValue' 
    # indicates that there was no fill value found for the dataset).
    #
    # The first element list corresponds to the 3 datasets in master_ordered_list[0]
    # and the second element list corresponds to the 2 datasets in master_ordered_list[1]
    #
    #  [[(8650,), '/Data_1HZ/DS_UTCTime_1', '/Data_1HZ/Geolocation/d_lat', '/Data_1HZ/Geolocation/d_lon',
    #   [(346000,), '/Data_40HZ/Elevation_Flags/elv_gauss_flg', '/Data_40HZ/Elevation_Flags/elv_cloud_flg']]
    #
    var_unit_value = []
#   var_unit_value.append([])
    
    # 
    # Iterate through the master ordered list and get their attributes
    #
    for i in (range (0, len(master_ordered_list))):
       var_unit_value.append([])
       #
       # We start in position 1 because position 0 is the array size
       #
       for j in (range (1, len(master_ordered_list[i]))):
        var_unit_value[i].append([])
       # 
       # Initialize the unit value with 0 and fill value with '_NoFillValue'
       # We will replace these values if we find unit values and fill values
       # later as we iterate over each dataset
################################################################################################      #
        var_unit_value[i][j-1].append(0)
        var_unit_value[i][j-1].append('_NoFillValue')

       # Put the data type in the third element of the variable attribute group
       #
#        print "MASTER_ORDERED_LIST[i][j].dtype is: {0}".format(hf.get(master_ordered_list[i][j]).dtype)
        var_unit_value[i][j-1].append(hf.get(master_ordered_list[i][j][1]).dtype.name)
#        for key in hf.get(master_ordered_list[i][j]).attrs.keys():
#           print ("KEY = {0}".format(key))
        for key, value in hf.get(master_ordered_list[i][j][1]).attrs.items():
#           print ("{0} = {1}".format(key,value))
            #
            # Not all datasets will have an attribute of units but if
            # we find one, store the unit value (actually we remove the
            # value used to initialize the units (0) and then insert
            # the attribute value. We process the fill value the same
            # way
            #
            ####################################################################################
            if (key == "units"):
                var_unit_value[i][j-1].remove(0)
                var_unit_value[i][j-1].insert(0,value.decode("utf-8"))

            if (key == "_FillValue"):
                var_unit_value[i][j-1].remove('_NoFillValue')
                #
                # Subtle difference between fill value attribute. In V4.0 data, fill value for some
                # ICESat-2 fill value attributes are stored as 1-element arrays but in V4.1, they
                # are scalars.
                #
                if (not np.isscalar(value)):
                   fill_val_attribute = value[0]
                else:
                   fill_val_attribute = value
                var_unit_value[i][j-1].insert(1,fill_val_attribute)

    if (debug == 'Y'):
       log.writeMessage("MASTER ORDERED LIST DATASET, UNITS, FILL_VALUE, DATATYPE: \n")
       for i in (range (0, len(master_ordered_list))):
          for j in (range (1, len(master_ordered_list[i]))):
             log.writeMessage(str(master_ordered_list[i][j][1]) + " " + str(var_unit_value[i][j-1]) + "\n") 

    
    # Now we start the ascii printing process
    # We will go through our master ordered list with the objective of
    # printing out all values for the first set of data
    # and then the second and so-on.
    #
    for list_index in range (0, len(master_ordered_list)):

       if (debug == 'Y'):
           log.writeMessage("LIST_INDEX IS: " + str(list_index) + "\n")

#
#   If we have a set of data that is multidimensional we will just skip it for now (NCR # 8053622)
#
#      if (len(master_ordered_list[list_index][0]) > 1):
#         if (debug == 'Y'):
#              print "multidimensional dataset encountered, skip it for now (NCR # 8053622)"
#         continue

#
#   If we have a set of data that is greater than 2 dimensions we will skip it
#
       if (len(master_ordered_list[list_index][0]) > 2):
          if (debug == 'Y'):
             log.writeMessage("multidimensional dataset > 2 dimensions " + "\n")
          continue


       contents = []

       #
       # Get the array size we are working with from the first dataset array
       #

       array_len = len(np.array(hf.get(master_ordered_list[list_index][1][1])))
       if (debug == 'Y'):
          log.writeMessage("array_len is: " + str(array_len) + "\n")
   
       #
       # We will build our contents container to contain all data for this group of
       # tabular ascii data. We build it by concatenating each dataset array
       #
#       contents = np.array(hf.get(master_ordered_list[list_index][1]))
       if (debug == 'Y'):
           log.writeMessage("FIRST ELEMENT: MASTER_ORDERED_LIST[list_index][1] is: " + master_ordered_list[list_index][1][1] + "\n")
       added_dimensions = 0;
       for i in range (1, len(master_ordered_list[list_index])):
           if (debug == 'Y'):
              log.writeMessage("NEXT ELEMENT: " + master_ordered_list[list_index][i][1] + "\n")
              log.writeMessage("shape: " + str(np.array(hf.get(master_ordered_list[list_index][i][1])).shape) + "\n")

#           contents = np.concatenate((contents,np.array(hf.get(master_ordered_list[list_index][i]))), axis=0)
           if (debug == 'Y'):
              log.writeMessage("SHAPE OF CONTENTS:  " + str(np.array(hf.get(master_ordered_list[list_index][i][1])).shape) + "\n")
            	
           if (master_ordered_list[list_index][i][0] > 0):
#              print(np.array(hf.get(master_ordered_list[list_index][i][1])))
              rebuilt_array = np.swapaxes(np.array(hf.get(master_ordered_list[list_index][i][1])),0,1)   
        

 #
       # We will keep track of added dimensions of the 2d dataset so we can print out the proper number
       # of columns (we do this by grabbing that first element of the tuple in the list that contains
       # the 2nd dimension. We subtract one because we just want to add the additional banded datasets
       # If this were a six banded dataset we would just add 5.
       #
              added_dimensions = added_dimensions + master_ordered_list[list_index][i][0] - 1;

       # We concantenate our dataset for each dimension
       #
              for dimension in range (0, master_ordered_list[list_index][i][0]):
                 contents = np.concatenate((contents,rebuilt_array[dimension]), axis=0)
              if (debug == 'Y'):
                   log.writeMessage("2D SHAPE CONTENTS" + str(contents.shape) + "\n")

           else:


#
       # Other wise just concatenate our 1d dataset as usual
       #
              contents = np.concatenate((contents,(np.array(hf.get(master_ordered_list[list_index][i][1])))), axis=0)

       if (debug == 'Y'):

          log.writeMessage("ADDED DIMENSIONS: " + str(added_dimensions) + "\n")
          log.writeMessage("SHAPE OF CONTENTS: " + str(contents.shape) + "\n")
   # Reshape the list into the (number of variables) X (number of items in each dataset) (taking into account
   # the added dimensions for the banded datasets).
 
       contents = contents.reshape(len(master_ordered_list[list_index])-1+added_dimensions,array_len)

   #
   # Reshape the list into the (number of variables) X (number of items in each dataset)
   #
    
       if (debug == 'Y'):
          log.writeMessage("SHAPE OF CONTENTS AFTER RESHAPE: " + str(contents.shape) + "\n")

   #
   # Print the header of data set names
   #
       for index in range (1, len(master_ordered_list[list_index])):
           if (master_ordered_list[list_index][index][0] > 0):
               for j in range (0, master_ordered_list[list_index][index][0]):
                 file.write(' {0:}[{1:}]'.format(master_ordered_list[list_index][index][1],j+1))
           else: 
               #
               # If we have a dataset length less than 26 characters and it might be a date/time value...
               #
               if (debug == 'Y'):
                  log.writeMessage("MASTER_ORDERED_LIST[LIST_INDEX][INDEX][1] " + str(master_ordered_list[list_index][index][1]) + "\n")
                  log.writeMessage("LEN of MASTER_ORDERED_LIST[LIST_INDEX][INDEX][1] " + str(dataset_len_list[master_ordered_list[list_index][index][1]]) + "\n")
                  log.writeMessage("VAR_UNIT_VALUE[LIST_INDEX][INDEX-1][0] " + str(var_unit_value[list_index][index-1][0]) + "\n")
               
               
               if ((dataset_len_list[master_ordered_list[list_index][index][1]] < date_time_length) and (re.search(r'seconds',str(var_unit_value[list_index][index-1][0])))):
                  #
                  # We will add some spaces for alignment...
                  #
                  file.write('{0:1}  '.format(" " * (date_time_length - dataset_len_list[master_ordered_list[list_index][index][1]] - 1)))
               file.write(' {0:}'.format(master_ordered_list[list_index][index][1]))
       file.write('\n')
   
       index = 0

   #
   # Loop over the dataset array sizes
   #
       for index in range (0, array_len):
   #
   # Within the larger data loop, we will print out the next element of each dataset
   # on one line
   #

# We maintain a dataset pointer to index into the list of datasets
           dataset_pointer = 0
       # If we have a banded dataset we will set a multi_dataset_counter to tick off
       # the times we print each banded dataset element
       #
           multi_dataset_counter = 0
       #
       # Here we are printing out each element including any added dimensions for
       # multi-banded (2d) datasets
       #
           for  i in range (0, len(master_ordered_list[list_index])-1+added_dimensions):
       #
       # If we have a two dimensional dataset (banded dataset) within our list, we will
       # increment our multi dataset counter. If we reach the last element we will reset
       # our multi dataset counter.
       #
               if (master_ordered_list[list_index][dataset_pointer+1][0] > 0):
                  multi_dataset_counter += 1
                  if (multi_dataset_counter == (master_ordered_list[list_index][dataset_pointer+1][0])):
                     multi_dataset_counter = 0
       #
       # Check to see if we have a potential fill value - rather complicated logic here:
       # Don't try to handle ancillary data for fill value processing...
       # If we have a fill value (!= '_NoFillValue)... and
       # and the fill value is not zero (we will print out zero itself if it is defined as a fill value...
       # and the current data items (contents[i,index] >= our fill value in the var_unit_value array)
       # or we are looking at a GLAS granule file and the contents is our fill_value we read from the GenAscii.ini configuration file...
       #
               log.writeMessage("WORKING ON: " 
                  + str(master_ordered_list[list_index]
                        [dataset_pointer+1][1]) + "\n")

               if ((not re.search(r'^\/ancillary_data',
                        str(master_ordered_list[list_index][dataset_pointer+1][1])))
                    and (var_unit_value[list_index][dataset_pointer][1] != '_NoFillValue')
                    and (int(var_unit_value[list_index][dataset_pointer][1]) != 0)
                    and (contents[i,index] >= float(var_unit_value[list_index][dataset_pointer][1]))) or (re.search(r'GLA',inputfile) and (contents[i,index] >= float(fill_value))):

                   dataset_length = dataset_len_list[master_ordered_list[list_index][dataset_pointer+1][1]]
       #
       # If we have a 2-dimension banded data set add 3 spaces to account for printing out the
       # multi banded dataset indices, i.e. [1], [2], etc.


                   if (master_ordered_list[list_index][dataset_pointer+1][0] > 0):
                      dataset_length += 3
                   write_blanks(file, dataset_length, 12)
                   file.write ('{0:12}'.format("<Fill_Value>"))
#
        # We will handle all multi dimensional arrays in this block
        #
#################################################################################
#               elif (len(master_ordered_list[list_index][0]) > 1):
#                   dataset_length = dataset_len_list[master_ordered_list[list_index][i+1]]
#                   write_blanks(file, dataset_lengthi, 17)
#                   for j in range (0, contents.shape[1]):
#                      if (var_unit_value[list_index][i][1] != '_NoFillValue') and (contents[index,j] == float(var_unit_value[list_index][i][1])):
#                         file.write ('{0:12} '.format("<Fill_Value>")),
##                      elif var_unit_value[list_index][i][0] == 'i':
#                      elif "int" in var_unit_value[list_index][i][2]:
#                         file.write('{0:d} '.format(contents[index,j]))
#                      else:
#                         file.write('{0:16.6f} '.format(contents[index,j]))
###################################################################################
       # If we have a time field, print it out as readable
       #


                     # The transmit time of each shot in the 1 second frame measured as UTC seconds elapsed since Jan 1 2000 12:00:00 UTC.
                     # This time has been derived from the GPS time accounting for leap seconds
                     #
                     # For GLAS data we need to add the offset
                     #

               #
               # Added a condition to this block to not attempt to convert seconds to time for items that
               # are strings (usually these strings show up in /ancillary_data for ATLAS products)
               #
               # Also we have a condition where some GLAS products have just seconds as their units which indicates a time field so we
               # have to check for units = seconds and make sure the product is a glas product by making sure GLA is a substring in the inputfile
               #
               elif ((var_unit_value[list_index][dataset_pointer][0] == 'seconds') and (re.search(r'GLA',inputfile))) or (var_unit_value[list_index][dataset_pointer][0] == 'seconds since 2000-01-01 12:00:00 UTC' and not isinstance(contents[i,index], str)):
                   t_sec = contents[i,index] + 946728000.0
                   tloc = time.gmtime(t_sec)
                   dataset_length = dataset_len_list[master_ordered_list[list_index][dataset_pointer+1][1]]
                   if (master_ordered_list[list_index][dataset_pointer+1][0] > 0):
                      dataset_length += 3
                   write_blanks(file, dataset_length, date_time_length)
                   file.write(time.strftime('  %m/%d/%Y %H:%M:%S', tloc))


                   #
                   # For ATLAS data (ICESat-2), we need to add the offset (seconds between 1/6/80 and 1/1/05: 788572813.0)
                   # and then also add in the time between 1/1/70 and 1/6/80 (315964783.0)
                   #

                                                                                                                   
               elif (isinstance (var_unit_value[list_index][dataset_pointer][0], str)) and re.match(r'[Ss]econds since (\d\d\d\d-\d\d-\d\d)', var_unit_value[list_index][dataset_pointer][0]):

                   match = re.search(r'\d{4}-\d{2}-\d{2}', var_unit_value[list_index][dataset_pointer][0])
                   unit_date = str(datetime.strptime(match.group(), '%Y-%m-%d').date())
                   epoch = unit_date+"T00.00.00.000000"
                   epochtime = datetime.strptime(epoch,format); 
                   tloc = epochtime + timedelta(seconds=float(contents[i,index]));
                   dataset_length = dataset_len_list[master_ordered_list[list_index][dataset_pointer+1][1]]
                   write_blanks(file, dataset_length, date_time_length);
                   file.write(tloc.strftime('  %m/%d/%Y %H:%M:%S.%f'))
               

               elif var_unit_value[list_index][dataset_pointer][0] == "joules":
                   dataset_length = dataset_len_list[master_ordered_list[list_index][dataset_pointer+1][1]]
                   if (master_ordered_list[list_index][dataset_pointer+1][0] > 0):
                      dataset_length += 3
                   write_blanks(file, dataset_length, 17)
                   file.write('{0:.2e}'.format(float(contents[i,index])))

               elif ((not re.search(r'int',var_unit_value[list_index][dataset_pointer][2])) and (not re.search(r'bytes',var_unit_value[list_index][dataset_pointer][2]))):
                   if (var_unit_value[list_index][dataset_pointer][0] == "seconds" and float(contents[i,index]) < 0.000001) or \
                      (var_unit_value[list_index][dataset_pointer][0] == "1" and float(contents[i,index]) < 0.000001):
                       dataset_length = dataset_len_list[master_ordered_list[list_index][dataset_pointer+1][1]]
                       if (master_ordered_list[list_index][dataset_pointer+1][0] > 0):
                          dataset_length += 3
                       write_blanks(file, dataset_length, 8)
#                       file.write('{0:.1e}'.format(float(contents[i,index].decode("utf-8"))))
                       file.write('{0:.1e}'.format(float(contents[i,index])))
                   else:
                       file.write('{0:16.6f}'.format(float(contents[i,index])))

               elif var_unit_value[list_index][dataset_pointer][0] == "1/(m-sr)":
                   dataset_length = dataset_len_list[master_ordered_list[list_index][dataset_pointer+1][1]]
                   if (master_ordered_list[list_index][dataset_pointer+1][0] > 0):
                      dataset_length += 3
                   write_blanks(file, dataset_length, 17)
                   file.write('{0:.3e}'.format(contents[i,index]))
               else:
                   dataset_length = dataset_len_list[master_ordered_list[list_index][dataset_pointer+1][1]]
#                   dataset_length = dataset_len_list[master_ordered_list[list_index][i+1]]
                   if (master_ordered_list[list_index][dataset_pointer+1][0] > 0):
                      dataset_length += 3
                   write_blanks(file, dataset_length, 16)
#                  print "VAR_UNIT_VALUE[LIST_INDEX][DATASET_POINTER][2] IS: ", var_unit_value[list_index][dataset_pointer][2]
#                  print "MASTER_ORDERED_LIST dataset causing the problem: IS: ", master_ordered_list[list_index][dataset_pointer+1][1]
                   #
                   # If we have an integer type, we want to make sure to print it out as such so 
                   # we have to convert the contents using the numpy astype function
                   #
                   
#                  print ('I, INDEX', i, index)
                   if (re.search(r'int',var_unit_value[list_index][dataset_pointer][2])):
#                     log.writeMessage("INT section\n")
                      file.write('{0:16d}'.format(int(float(contents[i,index]))))
                   #
                   # print out string values appropriately    
                   #
                   elif (re.search(r'string',var_unit_value[list_index][dataset_pointer][2])) or isinstance(contents[i,index], str):
#                     log.writeMessage("STRING section\n")
                      file.write('{0:16s}'.format(contents[i,index]))
                   #
                   # print out bytes values as strings
                   #
                   elif (re.search(r'bytes',var_unit_value[list_index][dataset_pointer][2])):
#                     log.writeMessage("BYTES section\n")
                      file.write('{0:16s}'.format(contents[i,index].decode("utf-8")))
                   #
                   # otherwise these are floats
                   #
                   else:
                      file.write('{0:16.6f}'.format(float(contents[i,index])))
               if (multi_dataset_counter == 0):
                  dataset_pointer += 1
       # write out a new line
           file.write('\n')
       file.write('\n')
file.close()
log.XMLReturnSuccess(inputfile, outputfile)
