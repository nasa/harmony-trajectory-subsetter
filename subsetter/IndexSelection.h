#ifndef IndexSelection_H 
#define IndexSelection_H
//
// IndexSelection encapsulates the temporal bounds and the spatial 
// bounding boxes into a single structure, bbox.
//
//

#include <map>
#include <iostream>
using namespace std;

class IndexSelection
{
    public:

        // ordered list of non-overlapping start/length pairs 
        map<long, long> bbox;

        // begin and end are used to limit the union of bounding boxes to a specified
        // region. they are determined by the temporal bounding box
        long begin;
        long end; // = start+length = ending index+1 



        //initialization with length of 1-d array for dimension scale
        IndexSelection(long maxlength) : maxsize(maxlength), begin(0), end(maxlength) {};
        long offset() {return begin;};

        long getMaxSize() {return maxsize;}

        long size() 
        {
            long size = 0;
            map<long, long>::iterator it;
            for (it=bbox.begin();it!=bbox.end();it++)
                size += it->second;
            if (!size) size=end-begin;                
            return size;
        }

        //add temporal restriction, assume the data is temporally continuous
        void addRestriction(long start, long length)
        {
            cout << "IndexSelection.addRestriction changing from (" << begin << "," << end << ") to (" 
                 << start << "," << start+length << ")" << endl; 
            begin = start;
            end = start+length;
            // reset the bbox map with the new temporal constraints in place
            map<long,long> original_bbox = bbox;
            bbox.erase(bbox.begin(), bbox.end());
            map<long, long>::iterator it = original_bbox.begin();
            for (;it != original_bbox.end();it++) addBox(it->first,it->second);
        }

        //union bounding box with existing ones within limits
        void addBox(long start, long length)
        {
            map<long, long>::iterator it;

            //limit bounding box to temporal region
            if (start < begin) 
            {
                length = length - (begin - start);
                start = begin;
            }
            if (start+length > end) length = end-start;


            //if bounding box does not exist in temporal selection do nothing
            if (length <= 0) return;

            cout << "IndexSelection.addBox Adding box " << start << " - " << length << endl;

            //union this box with all the others
            for (it=bbox.begin();it!=bbox.end();it++)
            {
                //starts before existing bounding box
                if (it->first > start) 
                {
                    //ends before start of existing bounding box add bounding box
                    if (it->first > start + length) bbox[start]=length;
                    else
                    //ends after existing bounding box ends
                    if (it->first + it->second <= start + length) 
                    {
                        bbox.erase(it);         //erase existing box 
                        addBox(start,length);   //add this one
                    }
                    else 
                    //if it ends before existing bounding box ends 
                    if (it->first + it->second > start + length)
                    {
                        long newlength = it->first + it->second - start;
                        bbox.erase(it);         //erase existing box 
                        bbox[start]=newlength;  //create unioned bounding box 
                    }    
                    return;
                }
                //if it starts after existing bounding box
                else if (it->first <= start)
                {
                    //if fully enclosed in an existing bounding box ignore
                    if (it->first + it->second >= start + length) return;
          
                    //if it comes after exiting bounding box go on to next one
                    if (it->first + it->second < start) continue;

                    //if it overlaps, remove existing bounding box and add this one 
                    if (it->first + it->second <= start+length)
                    {
                        long newstart = it->first;
                        long newlength = start + length - it->first;
                        bbox.erase(it);
                        addBox(newstart, newlength);
                        return;
                    }
                }
                         
            }

            //add bounding box if we haven't yet
            bbox[start]=length;
        }

        friend ostream& operator<<(ostream& out, IndexSelection& bbox)
        {
            map<long, long>::iterator it;
            out << "[" << bbox.begin << " ";
            for (it=bbox.bbox.begin();it!=bbox.bbox.end();it++) out << " (" << it->first << "," << it->second << ") ";
            out << " " << bbox.end << "]";
            return out; 
        }
    private:
        // size of 1-D dimension scale array
        long maxsize;

};
#endif

/*
int main()
{
    IndexSelection bbox(10, 190);
    while(1)
    {
        long start,length;
        cout << "enter start of bound : " ;
        cin >> start;
        cout << "enter length of bound : ";
        cin >> length;
        bbox.addBox(start, length);
        cout << "Bounding box is now " << bbox << endl;
    }
    return 0;
};
*/
