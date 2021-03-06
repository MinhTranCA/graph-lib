/**
 * @file MultipleIPFPGraphEditDistance.h
 * @author Evariste <<evariste.daller@unicaen.fr>>
 * @version     Jun  9 2017
 *
 */

#ifndef __MULTISTARTREFINEMENTGED_H__
#define __MULTISTARTREFINEMENTGED_H__

#ifdef _OPENMP
  #include <omp.h>
#endif

#include <sys/time.h>
#include <list>
#include "GraphEditDistance.h"
#include "MultistartMappingRefinement.h"



/**
 * @brief Implements a MultistartMappingRefinement method adapted to the GED problem
 */
template<class NodeAttribute, class EdgeAttribute>
class MultistartRefinementGraphEditDistance:
  public virtual GraphEditDistance<NodeAttribute,EdgeAttribute>,
  public MultistartMappingRefinement<NodeAttribute, EdgeAttribute>
{

protected:

  MappingRefinement<NodeAttribute, EdgeAttribute> * method; //!< Storage of a predefined refinement method
  std::list<int*> refinedReverseMappings; //!< Storage of the reverse mappings needed for the (n+1)*(m+1) GED modelisation

  bool cleanMethod; //!< Delete the method in the destructor if true

public:


  /**
   * @brief Get the best possible mapping for the GED, with the given initialization and refinement methods
   */
  virtual void getOptimalMapping( Graph<NodeAttribute,EdgeAttribute> * g1,
                                  Graph<NodeAttribute,EdgeAttribute> * g2,
                                  int * G1_to_G2, int * G2_to_G1 );


  virtual void getBestMappingFromSet( MappingRefinement<NodeAttribute, EdgeAttribute> * algorithm,
                                      Graph<NodeAttribute,EdgeAttribute> * g1,
                                      Graph<NodeAttribute,EdgeAttribute> * g2,
                                      int * G1_to_G2, int * G2_to_G1,
                                      std::list<int*>& mappings );


  /**
   * First Constructor
   */
  MultistartRefinementGraphEditDistance( EditDistanceCost<NodeAttribute,EdgeAttribute> * costFunction,
                                 MappingGenerator<NodeAttribute,EdgeAttribute> * gen,
                                 int n_edit_paths,
                                 MappingRefinement<NodeAttribute, EdgeAttribute> * algorithm
                               ):
    GraphEditDistance<NodeAttribute,EdgeAttribute> (costFunction),
    MultistartMappingRefinement<NodeAttribute, EdgeAttribute> (gen, n_edit_paths),
    method(algorithm),
    cleanMethod(false)
  {}


  MultistartRefinementGraphEditDistance(
                        const MultistartRefinementGraphEditDistance<NodeAttribute, EdgeAttribute>& other
                        ):
    GraphEditDistance<NodeAttribute,EdgeAttribute> (other.cf),
    MultistartMappingRefinement<NodeAttribute, EdgeAttribute> (other.initGen->clone(), other.k),
    method(other.method->clone()),
    cleanMethod(true)
  {}


  ~MultistartRefinementGraphEditDistance(){
    if (cleanMethod){
      delete method;
      delete this->initGen;
    }
  }


  /**
   * @brief Returns refined mappings generated by the internal generator \ref initGen
   *
   *  The refinement method is \ref method
   */
  virtual const std::list<int*>& 
  getBetterMappings( Graph<NodeAttribute,EdgeAttribute> * g1,
                     Graph<NodeAttribute,EdgeAttribute> * g2 );


  /**
   * @brief Returns refined mappings from the ones given in parameter, according to the method \method
   *
   *   Note that the returned mappings are the forward mappings of size (n+1). To get the 
   *   (m+1) reverse mappings denoted by G2_to_G1, use the method \ref getReverseMappings.
   *
   * @param  g1          First graph
   * @param  g2          Second graph
   * @param  mapping     a list of arrays representing initial mappings
   * @note   Forward and reverse mappings are allocated on the heap and memory management is left to the user
   * @see getBetterMappings getReverseMappings
   */
  virtual const std::list<int*>&
  getBetterMappingsFromSet( Graph<NodeAttribute,EdgeAttribute> * g1,
                            Graph<NodeAttribute,EdgeAttribute> * g2,
                            std::list<int*>& mappings );


  /**
   * @brief Redefinition of \ref MultistartMappingRefinement::getBetterMappingsFromSet for the GED problem
   *
   *   Note that the returned mappings are the forward mappings of size (n+1). To get the 
   *   (m+1) reverse mappings denoted by G2_to_G1, use the method \ref getReverseMappings.
   *
   * @param  algorithm   the refinement method
   * @param  g1          First graph
   * @param  g2          Second graph
   * @param  mapping     a list of arrays representing initial mappings
   * @note   Forward and reverse mappings are allocated on the heap and memory management is left to the user
   * @see getBetterMappings getReverseMappings
   */
  virtual const std::list<int*>&
  getBetterMappingsFromSet( MappingRefinement<NodeAttribute, EdgeAttribute> * algorithm,
                            Graph<NodeAttribute,EdgeAttribute> * g1,
                            Graph<NodeAttribute,EdgeAttribute> * g2,
                            std::list<int*>& mappings );

  /**
   * @brief Returns the last reverse mappings G2_to_G1 computed from \ref getBetterMappingsFromSet or \ref getBetterMappings
   */
  std::list<int*>& getReverseMappings(){ return refinedReverseMappings; }


  /**
   * Clone
   */
   virtual MultistartRefinementGraphEditDistance<NodeAttribute, EdgeAttribute>* clone() const {
     return new MultistartRefinementGraphEditDistance<NodeAttribute, EdgeAttribute>(*this);
   }

};

//---


template<class NodeAttribute, class EdgeAttribute>
void MultistartRefinementGraphEditDistance<NodeAttribute, EdgeAttribute>::
getOptimalMapping( Graph<NodeAttribute,EdgeAttribute> * g1,
                   Graph<NodeAttribute,EdgeAttribute> * g2,
                   int * G1_to_G2, int * G2_to_G1)
{
  this->getBestMapping(method, g1, g2, G1_to_G2, G2_to_G1);
}



template<class NodeAttribute, class EdgeAttribute>
void MultistartRefinementGraphEditDistance<NodeAttribute, EdgeAttribute>::
getBestMappingFromSet( MappingRefinement<NodeAttribute, EdgeAttribute> * algorithm,
                Graph<NodeAttribute,EdgeAttribute> * g1,
                Graph<NodeAttribute,EdgeAttribute> * g2,
                int * G1_to_G2, int * G2_to_G1,
                std::list<int*>& mappings )
{
  struct timeval  tv1, tv2;
  int n = g1->Size();
  int m = g2->Size();

  typename std::list<int*>::const_iterator it;
  double cost = -1;
  double ncost;


  // Multithread
  #ifdef _OPENMP
    gettimeofday(&tv1, NULL);
    int** arrayMappings = new int*[mappings.size()];
    int* arrayCosts = new int[mappings.size()];
    int* arrayLocal_G1_to_G2 = new int[(n+1) * mappings.size()];
    int* arrayLocal_G2_to_G1 = new int[(m+1) * mappings.size()];

    int i=0; for (it=mappings.begin(); it!=mappings.end(); it++){
      arrayMappings[i] = *it;
      i++;
    }

    //omp_set_dynamic(0);
    //omp_set_num_threads(4);
    #pragma omp parallel for schedule(dynamic) //private(tid, i, j, ncost, ipfpGed )
    for (unsigned int tid=0; tid<mappings.size(); tid++){
      int* lsapMapping = arrayMappings[tid];
      int* local_G1_to_G2 = &(arrayLocal_G1_to_G2[tid*(n+1)]);
      int* local_G2_to_G1 = &(arrayLocal_G2_to_G1[tid*(m+1)]);

  // Sequential
  #else
    int* local_G1_to_G2 = new int[n+1];
    int* local_G2_to_G1 = new int[m+1];

    double t_acc = 0; // accumulated time
    for (it=mappings.begin(); it!=mappings.end(); it++){
      gettimeofday(&tv1, NULL);
      int* lsapMapping = *it;
  #endif

    // computation of G1_to_G2 and G2_to_G1
    for (int j=0; j<m; j++){ // connect all to epsilon by default
      local_G2_to_G1[j] = n;
    }

    for (int i=0; i<n; i++){
      if (lsapMapping[i] >= m)
        local_G1_to_G2[i] = m; // i -> epsilon
      else{
        local_G1_to_G2[i] = lsapMapping[i];
        local_G2_to_G1[lsapMapping[i]] = i;
      }
    }

    for (int j=0; j<m; j++){
      if (lsapMapping[n+j] < m){
        local_G2_to_G1[j] = n; // epsilon -> j
      }
    }

    MappingRefinement<NodeAttribute, EdgeAttribute> * local_method;

    #ifdef _OPENMP
      local_method = algorithm->clone();
    #else
      local_method = algorithm;
    #endif
    
    local_method->getBetterMapping(g1, g2, local_G1_to_G2, local_G2_to_G1);
    ncost = local_method->mappingCost(g1, g2, local_G1_to_G2, local_G2_to_G1);


    // Multithread
    #ifdef _OPENMP
      // save the approx cost
      arrayCosts[tid] = ncost;
      // _distances_[tid] = ncost;
     
      // Delete cloned local method
      delete local_method;

    // Sequential
    #else
      // if ncost is better : save the mapping and the cost
      if (cost > ncost || cost == -1){
        cost = ncost;
        for (int i=0; i<n; i++) G1_to_G2[i] = local_G1_to_G2[i];
        for (int j=0; j<m; j++) G2_to_G1[j] = local_G2_to_G1[j];
      }
      gettimeofday(&tv2, NULL);
      t_acc += ((double)(tv2.tv_usec - tv1.tv_usec)/1000000 + (double)(tv2.tv_sec - tv1.tv_sec));

    #endif

  } //end for

  // Multithread : Reduction
  #ifdef _OPENMP
    gettimeofday(&tv2, NULL);
    
    gettimeofday(&tv1, NULL);

    int i_optim;
    for (unsigned int i=0; i<mappings.size(); i++){
      if (cost > arrayCosts[i] || cost == -1){
         cost = arrayCosts[i];
         i_optim = i;
      }
    }
    for (int i=0; i<n; i++) G1_to_G2[i] = arrayLocal_G1_to_G2[i_optim*(n+1)+i];
    for (int j=0; j<m; j++) G2_to_G1[j] = arrayLocal_G2_to_G1[i_optim*(m+1)+j];

    // To match the output format size in XPs
    //for (int i=mappings.size(); i<k; i++) _distances_[i] = 9999;

    gettimeofday(&tv2, NULL);
    //_xp_out_ <<  ((double)(tv2.tv_usec - tv1.tv_usec)/1000000 + (double)(tv2.tv_sec - tv1.tv_sec)) << ", ";
    //_xp_out_ << ((float)t) / CLOCKS_PER_SEC << ", ";

    delete[] arrayLocal_G1_to_G2;
    delete[] arrayLocal_G2_to_G1;
    delete[] arrayCosts;
    delete[] arrayMappings;

  // Sequential : deletes
  #else

    delete [] local_G1_to_G2;
    delete [] local_G2_to_G1;

  #endif

}




template<class NodeAttribute, class EdgeAttribute>
const std::list<int*>& MultistartRefinementGraphEditDistance<NodeAttribute, EdgeAttribute>::
getBetterMappings( Graph<NodeAttribute,EdgeAttribute> * g1,
                   Graph<NodeAttribute,EdgeAttribute> * g2 )
{
  std::list<int*> mappings = this->initGen->getMappings(g1, g2, this->k);
  const std::list<int*>& refined = this->getBetterMappingsFromSet(method, g1, g2, mappings);

  // Delete original (bipartite) mappings
  for (std::list<int*>::iterator it=mappings.begin();
       it != mappings.end();   it++)
    delete [] *it;

  return refined;
}




template<class NodeAttribute, class EdgeAttribute>
const std::list<int*>& MultistartRefinementGraphEditDistance<NodeAttribute, EdgeAttribute>::
getBetterMappingsFromSet( Graph<NodeAttribute,EdgeAttribute> * g1,
                          Graph<NodeAttribute,EdgeAttribute> * g2,
                          std::list<int*>& mappings )
{
  return getBetterMappingsFromSet( method, g1, g2, mappings);
}



template<class NodeAttribute, class EdgeAttribute>
const std::list<int*>& MultistartRefinementGraphEditDistance<NodeAttribute, EdgeAttribute>::
getBetterMappingsFromSet( MappingRefinement<NodeAttribute, EdgeAttribute> * algorithm,
                          Graph<NodeAttribute,EdgeAttribute> * g1,
                          Graph<NodeAttribute,EdgeAttribute> * g2,
                          std::list<int*>& mappings )
{
  int n = g1->Size();
  int m = g2->Size();

  typename std::list<int*>::const_iterator it;

  int** arrayLocal_G1_to_G2 = new int*[mappings.size()]; // indexation of local G1toG2
  int** arrayLocal_G2_to_G1 = new int*[mappings.size()];

  for (unsigned int i=0; i<mappings.size(); i++){
     arrayLocal_G1_to_G2[i] = new int[n+1];
     arrayLocal_G2_to_G1[i] = new int[m+1];
  }

  // Multithread
  #ifdef _OPENMP
    int** arrayMappings = new int*[mappings.size()];

    int i=0; for (it=mappings.begin(); it!=mappings.end(); it++){
      arrayMappings[i] = *it;
      i++;
    }

    //omp_set_dynamic(0);
    //omp_set_num_threads(4);
    // Set a dynamic scheduler : algorithms can have different execution time given different initial mappings
    #pragma omp parallel for schedule(dynamic) //private(tid, i, j, ncost, ipfpGed )
    for (unsigned int tid=0; tid<mappings.size(); tid++){
      int* lsapMapping = arrayMappings[tid];

  // Sequential
  #else
    unsigned int tid=0;
    for (it=mappings.begin(); it!=mappings.end(); it++){
      int* lsapMapping = *it;
  #endif
  
  
      int* local_G1_to_G2 = arrayLocal_G1_to_G2[tid];
      int* local_G2_to_G1 = arrayLocal_G2_to_G1[tid];

      // computation of G1_to_G2 and G2_to_G1
      for (int j=0; j<m; j++){ // connect all to epsilon by default
	local_G2_to_G1[j] = n;
      }
      
      for (int i=0; i<n; i++){
	if (lsapMapping[i] >= m)
	  local_G1_to_G2[i] = m; // i -> epsilon
	else{
	  local_G1_to_G2[i] = lsapMapping[i];
	  local_G2_to_G1[lsapMapping[i]] = i;
	}
      }
      
      for (int j=0; j<m; j++){
	if (lsapMapping[n+j] < m){
	  local_G2_to_G1[j] = n; // epsilon -> j
	}
      }

    
      MappingRefinement<NodeAttribute, EdgeAttribute> * local_method;
    
      #ifdef _OPENMP
        local_method = algorithm->clone();
      #else
        local_method = algorithm;
        tid++; //prepare the next iteration
      #endif
    
      local_method->getBetterMapping(g1, g2, local_G1_to_G2, local_G2_to_G1, true);

      #ifdef _OPENMP
        delete local_method;
      #endif

    } //end for

    // Reduction - indexation of the list
    this->refinedMappings.clear(); //G1_to_G2 in refinedMappings
    this->refinedReverseMappings.clear(); //G2_to_G1 in refinedReverseMappings
    int _i_=0;
    for (it=mappings.begin(); it!=mappings.end(); it++){
      this->refinedMappings.push_back(arrayLocal_G1_to_G2[_i_]);
      refinedReverseMappings.push_back(arrayLocal_G2_to_G1[_i_]);
      _i_++;
    }

   #ifdef _OPENMP
    delete[] arrayMappings;
   #endif
   delete[] arrayLocal_G1_to_G2;
   delete[] arrayLocal_G2_to_G1;
   
   return this->refinedMappings;
}
#endif // __MULTISTARTREFINEMENTGED_H__
