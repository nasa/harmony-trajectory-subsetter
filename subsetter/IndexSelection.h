#ifndef IndexSelection_H 
#define IndexSelection_H
//
// IndexSelection encapsulates the temporal bounds and the spatial
// bounding boxes into a single structure - selected segments of a 1D
// trajectory - start-index and length.
//

#include <map>
#include <iostream>


class IndexSelection
{
    public:

        // Ordered list of non-overlapping start/length pairs
        std::map<long, long> segments;

        // These constraints represent the lowest and highest possible index
        // value in the dataset - essentially it constrains the subset segments 
        // to the size of the dataset.
        long minIndexStart;
        long maxIndexEnd; // start+length = ending index+1

        // Initialization, typically with the length of 1-d array for dimension scale
        IndexSelection(long maxlength) :
                maxsize(maxlength),
                minIndexStart(0),
                maxIndexEnd(maxlength)
        {};

        long offset() 
            {return minIndexStart;};

        long getMaxSize() 
            {return maxsize;};

        long size() 
        {
            long size = 0;
            std::map<long, long>::iterator it;
            for (it=segments.begin(); it != segments.end(); it++)
                size += it->second;
            if (!size) size= maxIndexEnd - minIndexStart;
            return size;
        }

        // Add restriction - assume the data is continuous, so update the
        // subset constraints using the input start and length.
        void addRestriction(long newStart, long newLength)
        {
            std::cout << "IndexSelection.addRestriction changing from (" << minIndexStart << "," << maxIndexEnd << ") to ("
                 << newStart << "," << newStart+newLength << ")" << std::endl;
            minIndexStart = newStart;
            maxIndexEnd = newStart + newLength;
            // Reset the segments map with the new constraints in place.
            std::map<long,long> original_segments = segments;
            segments.erase(segments.begin(), segments.end());
            std::map<long, long>::iterator it;
            for (it = original_segments.begin(); it != original_segments.end(); it++) 
                addSegment(it->first, it->second);
        }

        // Add segment:
        //   union new segment with existing segments,
        //   and limit to within restrictions
        void addSegment(long newStart, long newLength)
        {
            std::map<long, long>::reverse_iterator it;

            // Check if segment start and length are within the bounds
            // of the photon dataset.
            // If they aren't, adjust start and/length so they are within bounds.
            if (newStart < minIndexStart)
            {
                newLength = newLength - (minIndexStart - newStart);
                newStart = minIndexStart;
            }
            if (newStart+newLength > maxIndexEnd)
            {
                newLength = maxIndexEnd - newStart;
            }

            // If bounding segment does not exist in subset selection do nothing.
            if (newLength <= 0)
            {
                return;
            }

            std::cout << "\tAdding segment (" << newStart << ", " << newLength << ")." << std::endl;

            // Potentially combine this segment with any existing overlapping segments
            //  [ ] - existing segment
            //  { } - new segment
            //   _  - content
            //   +  - joined
            // Sweep from end of list, check backwards from end of existing items

            long newEnd = newStart + newLength - 1;
            // Scan in reverse, optimized for adding to the end (the most typical use-case)
            // Note early loop/function exits, once an insertion point for new segment is found.
            for (it=segments.rbegin(); it != segments.rend(); it++)
            {
                // With reverse_iterator ---------------------------------------------------
                long existingStart = it->first;
                long existingLength = it->second;
                long existingEnd = existingStart + existingLength - 1;

                // Case A: New segment ends at or after existing segment
                if (newStart + newLength - 1 >= existingEnd)
                {
                    // Case A1: [ _ ] _ { _ } - if new segment starts after existing segment;
                    // (following, no overlap) a common use-case - add segment to end of list
                    if (newStart > (existingEnd + 1))
                    {
                        // Add segment as a new segment at end.
                        segments[newStart] = newLength;
                        // Note any intervening following segments have already been caught by this loop
                        return;
                    }
                    // Case A2: [ _ { _ ] _ } - new segment extends existing segment
                    else if (newStart >= existingStart)
                    {
                        // Reset length of this segment, no need to add segments
                        it->second = newStart + newLength - existingStart;
                        // Note any overlapping following segments have already been caught by this loop
                        return;
                    }
                    // Case A3: { _ [ _ ] _ } - new segment starts before existing segment starts (enclosing)
                    else
                    {
                        segments.erase( --(it.base()) );   // erase existing segment
                        addSegment(newStart, newLength);       // add this one,
                        // using recursion to catch any earlier potentially overlapping segments
                        return;
                    }
                }

                // Case B: New segment ends before the existing segment ends
                else // if (newEnd < existingEnd) // else to if above
                {
                    // Case B1: [ _ { _ } _ ] - new segment fully enclosed in existing segment - new segment starts before start of existing segment
                    if (newStart >= existingStart)
                    {
                        return;   // do nothing; existing segment in list already covers this case.
                    }

                    // Case B2: { _ [ _ } _ ] - new segment starts before existing segment ends
                    else if ((newEnd + 1) >= existingStart)
                    {
                        newLength = existingEnd - newStart + 1;   // revised length value
                        segments.erase( --(it.base()) ); // erase existing segment
                        addSegment(newStart, newLength);       // add this one
                        // Using recursion to catch any earlier potentially overlapping segments
                        return;  // exist loop
                    }
                }
            }

            // Add segment if we haven't yet (iterator was empty, any non-empty loop will find a return case)
            segments[newStart] = newLength;
        }

        friend std::ostream& operator<<(std::ostream& out, IndexSelection& selection)
        {
            std::map<long, long>::iterator it;
            out << "[" << " ";
            for (it=selection.segments.begin(); it != selection.segments.end(); it++) out << " (" << it->first << "," << it->second << ") ";
            out << " " << "]";
            return out; 
        }
    private:
        // size of 1-D dimension scale array
        long maxsize;

};
#endif