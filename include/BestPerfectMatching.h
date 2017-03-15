/**
 * @file bestPerfectMatchin.h
 * @author Évariste <<21511575@etu.unicaen.fr>>
 *
 * @todo EnumPerfectMatching and KBestPerfectMatching
 */

#ifndef __BESTPERFECTMATCHING_H__
#define __BESTPERFECTMATCHING_H__

#include <Eigen/Dense>
#include <vector>
#include <stack>


/**
 * @class BipartiteSCC
 * @author evariste
 * @date 14/03/17
 * @file BestPerfectMatching.h
 * @brief Denote the presence of nodes in a bipartite-graph
 *      related strongly connected component (SCC)
 *
 * @long Describe a SCC in a bipartite graph $G=(X \cup Y, E)$.
 *      * $\forall 0 < i \in < card(X)$, <code>u[i]</code> iff $x_i$ is in the SCC
 *      * $\forall 0 < j \in < card(Y)$, <code>v[j]</code> iff $y_j$ is in the SCC
 */
class BipartiteSCC{

public:

  std::vector<bool> u;
  std::vector<bool> v;

public:

  BipartiteSCC(unsigned int size_u, unsigned int size_v);

};



/**
 * @class BestPerfectMatching
 * @author evariste
 * @date 14/03/17
 * @file BestPerfectMatching.h
 * @brief
 */
class BestPerfectMatching{

private:

  // Ressources for Tarjan (bipartite)

  static int num;       //!< num of the current node
  static int access;    //!< numAccess of the current node

  static std::vector<int> vnum;     //!< list of number of nodes (first X then Y) @see offset
  static std::vector<int> vaccess;
  static std::vector<bool> instack;
  static std::stack<int> tarjanStack;
  static std::vector<BipartiteSCC> scc;

  //static bool inX; //!< true if we are looking in X, false for Y
  static int offset; //!< offset to access the right subset (0 if X, n if Y)
  static int offsize; //!< size of the offset

private:
  // Iter of Tarjan
  static void
  strongConnect( const Eigen::MatrixXi& gm, int v );

public:

  /**
   * @brief Find all the strongly connected components of the bipartite
   *      graph denoted by gm ($G=(X \cup Y, E)$)
   * @param gm  Adjacency matrix with $\card(X)$ rows and $\card(Y)
   *      columns with :
   *      * $gm_{ij} =  1$ iff an edge exists from $x_i$ to $y_i$
   *      * $gm_{ij} = -1$ iff an edge exists from $y_j$ to $x_i$
   *      * $gm_{ij] =  0$ iff there is no edge between $x_i$ and $x_j$
   * @return  The SCCs in the graph
   * @see BipartiteSCC
   */
  static const std::vector<BipartiteSCC>&
  findSCC( const Eigen::MatrixXi& gm );


  /**
   * @brief Removes all edges in the graph denoted by gm that are
   *      out a scc (edges between SCC).
   * @param gm  [inout] The graph
   * @param scc List of the SCCs in the graph
   * @see findSCC
   */
  static void
  rmUnnecessaryEdges( Eigen::MatrixXi& gm, const std::vector<BipartiteSCC>& scc );


  //TODO EnumPerfectMatching
  //TODO KBestPerfectMatching
};


#endif
