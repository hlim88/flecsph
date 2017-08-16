/*~--------------------------------------------------------------------------~*
 * Copyright (c) 2017 Los Alamos National Security, LLC
 * All rights reserved.
 *~--------------------------------------------------------------------------~*/

 /*~--------------------------------------------------------------------------~*
 * 
 * /@@@@@@@@  @@           @@@@@@   @@@@@@@@ @@@@@@@  @@      @@
 * /@@/////  /@@          @@////@@ @@////// /@@////@@/@@     /@@
 * /@@       /@@  @@@@@  @@    // /@@       /@@   /@@/@@     /@@
 * /@@@@@@@  /@@ @@///@@/@@       /@@@@@@@@@/@@@@@@@ /@@@@@@@@@@
 * /@@////   /@@/@@@@@@@/@@       ////////@@/@@////  /@@//////@@
 * /@@       /@@/@@//// //@@    @@       /@@/@@      /@@     /@@
 * /@@       @@@//@@@@@@ //@@@@@@  @@@@@@@@ /@@      /@@     /@@
 * //       ///  //////   //////  ////////  //       //      //  
 *
 *~--------------------------------------------------------------------------~*/

/**
 * @file tree.h
 * @author Julien Loiseau
 * @date April 2017
 * @brief Description of the tree policy used in the tree_topology from FleCSI. 
 */

#ifndef tree_h
#define tree_h

#include <vector>

#include "flecsi/topology/tree_topology.h"
#include "flecsi/geometry/point.h"
#include "flecsi/geometry/space_vector.h"

#include "body.h"

using namespace flecsi;

namespace flecsi{
namespace execution{
void specialization_driver(int argc, char * argv[]);
void driver(int argc, char*argv[]); 
} // namespace execution
} // namespace flecsi 

struct body_holder_mpi_t{
  static const size_t dimension = gdimension;
  using element_t = type_t; 
  using point_t = flecsi::point<element_t, dimension>;

  point_t position; 
  int owner; 
  double mass;
  int64_t id; 
};

class tree_policy{
public:
  using tree_t = flecsi::topology::tree_topology<tree_policy>;
  using branch_int_t = uint64_t;
  static const size_t dimension = gdimension;
  using element_t = type_t; 
  using point_t = flecsi::point<element_t, dimension>;
  using space_vector_t = flecsi::space_vector<element_t,dimension>;

  class body_holder : 
    public flecsi::topology::tree_entity<branch_int_t,dimension>{
  
  public: 

    body_holder(point_t position,
        body * bodyptr,
        int owner,
        element_t mass,
	int64_t id
        )
      :position_(position),bodyptr_(bodyptr),owner_(owner),mass_(mass),
	id_(id)
    {
      if(bodyptr_==nullptr)
        locality_ = NONLOCAL;
      else
        locality_ = EXCL;
    };

    body_holder()
      :position_(point_t{0,0,0}),
       bodyptr_(nullptr),
       owner_(-1),
       mass_(0.0),
       id_(int64_t(0))
    {
      locality_ = NONLOCAL;
    };

    // Function used in the tree structure 
    const point_t& coordinates() const {return position_;}
    const point_t& getPosition() const {return position_;}
    body* getBody(){return bodyptr_;};
    int getOwner(){return owner_;};
    element_t getMass(){return mass_;};
    int64_t getId(){return id_;}; 

    void setBody(body * bodyptr){bodyptr_ = bodyptr;};
    void setPosition(point_t position){position_ = position;};
    void setId(int64_t id){id_ = id;}; 

    friend std::ostream& operator<<(std::ostream& os, const body_holder& b){
      os << std::setprecision(10);
      os << "Holder. Pos: " <<b.position_ << " Mass: "<< b.mass_ << " "; 
      if(b.locality_ == LOCAL || b.locality_ == EXCL || b.locality_ == SHARED)
      {
        os<< "LOCAL";
      }else{
        os << "NONLOCAL";
      }
      os << " owner: " << b.owner_;
      os << " id: " << b.id_; 
      return os;
    }  

  private:
    // Should be replace by the key corresponding to the entity 
    point_t position_;
    body * bodyptr_; 
    int owner_;

    // Add this for COM in the current version
    element_t mass_;
    // Id of the particle behind
    int64_t id_;
  };
    
  using entity_t = body_holder;

  /**
   * Class entity_key_t used to represent the key of a body. 
   * The right way should be to add this informations directly inside 
   * the body_holder. 
   * With this information we should avoid the positions of the holder.
   * This is exactly the same representation as a branch but here 
   * the max depth is always used. 
   */
  class entity_key_t
  {
    public:
      using int_t = uint64_t;
      static const size_t dimension = gdimension;
      static constexpr size_t bits = sizeof(int_t) * 8;
      static constexpr size_t max_depth = (bits - 1)/dimension;
      using element_t = type_t; 

      entity_key_t()
      : id_(0)
      {}

      entity_key_t(
        //const std::array<point_t, 2>& range_,
        const point_t& p)
      : id_(int_t(1) << ((max_depth * dimension) + ((bits - 1) % dimension)))
      {
        std::array<int_t, dimension> coords;
        for(size_t i = 0; i < dimension; ++i)
        {
          element_t min = range_[0][i];
          element_t scale = range_[1][i] - min;
          coords[i] = (p[i] - min)/scale * (int_t(1) << (bits - 1)/dimension);
        }
        size_t k = 0;
        for(size_t i = 0; i < max_depth; ++i)
        {
          for(size_t j = 0; j < dimension; ++j)
          {
            int_t bit = (coords[j] & int_t(1) << i) >> i;
            id_ |= bit << (k * dimension + j);
          }
          ++k;
        }
      }
      
      //static 
      //std::array<point_t,2> range_;

      static
      void  
      set_range(std::array<point_t,2>& range){
        range_ = range; 
      } 

      constexpr entity_key_t(const entity_key_t& bid)
      : id_(bid.id_)
      {}

      static 
      constexpr
      entity_key_t
      null()
      {
        return entity_key_t(0);
      }

      int_t 
      truncate_value(int depth)
      {
        return id_ >> dimension*depth;
      }

      constexpr
      bool 
      is_null() const
      {
        return id_ == int_t(0);
      }

      entity_key_t&
      operator=(
        const entity_key_t& ek) 
      {
        id_ = ek.id_; 
        return *this;
      }

      constexpr
      bool 
      operator==(
        const entity_key_t& ek
      ) const 
    {
      return id_ == ek.id_; 
    }

    constexpr 
    bool 
    operator!=(
      const entity_key_t& ek
    ) const
    {
      return id_ != ek.id_; 
    }

    // Switch representation base on dimension 
    void
    output_(
      std::ostream& ostr
    ) const
    {
      // TODO change for others dimensions
      if(dimension == 3){
        ostr << std::oct << id_ << std::dec;
      }else if(dimension == 1){
        ostr << std::bitset<64>(id_);
      }else{
        // For dimension 2, display base 4
        ostr << "Dimension not handled";
      }
      // Old display group of bits based on the dimension
      //constexpr int_t mask = ((int_t(1) << dimension) - 1) << bits - dimension;
      //size_t d = max_depth;
      //int_t id = id_;
      //
      //while((id & mask) == int_t(0))
      //{
      //  --d;
      //  id <<= dimension;
      //}
      //if(d == 0)
      //{
      //  ostr << "<root>";
      //  return;
      //}
      //id <<= 1 + (bits - 1) % dimension;
      //for(size_t i = 1; i <= d; ++i)
      //{
      //  int_t val = (id & mask) >> (bits - dimension);
      //  ostr << std::oct << val << std::dec; 
      //  //ostr << i << ":" << std::bitset<dimension>(val) << " ";
      //  id <<= dimension;
      //}
    }

    bool
    operator<(
      const entity_key_t& bid
    ) const
    {
      return id_ < bid.id_;
    }
  
    bool
    operator>(
      const entity_key_t& bid
    ) const
    {
      return id_ > bid.id_;
    }
  
    bool
    operator<=(
      const entity_key_t& bid
    ) const
    {
      return id_ <= bid.id_;
    }
    
    bool
    operator>=(
      const entity_key_t& bid
    ) const
    {
      return id_ >= bid.id_;
    }

    entity_key_t 
    operator/(const int div){
      return entity_key_t(id_/div);
    }

    entity_key_t
    operator+(const entity_key_t& oth )
    {
      return entity_key_t(id_+oth.id_);
    }

    entity_key_t 
    operator-(const entity_key_t& oth)
    {
      return entity_key_t(id_-oth.id_);
    }

    // The first possible key 10000....
    static 
    constexpr
    entity_key_t
    first_key()
    {
      return entity_key_t(int_t(1) << 
          ((max_depth*dimension)+((bits-1)%dimension)));
    }

    // The last key 1777..., should be modified using not bit operation
    static
    constexpr
    entity_key_t
    last_key()
    {
      return entity_key_t(~int_t(0) >> (bits-(1+max_depth)*dimension
            +(gdimension-1)));      
    }
  
    int_t 
    value()
    {
      return id_;
    }
  
  private:
    int_t id_;
    static std::array<point_t,2> range_;

    constexpr
    entity_key_t(
      int_t id
    )
    :id_(id)
    {}
  };

  class branch : public flecsi::topology::tree_branch<branch_int_t,dimension,
  double>{
  public:
    branch(){}

    //void insert(body_holder* ent)

    void insert(body_holder* ent){
      // Check if same id in the branch 
      entity_key_t nkey = entity_key_t(ent->coordinates()); 
      for(auto& e: ents_)
      { 
        if(nkey == entity_key_t(e.front()->coordinates())){
          //std::cout<<"SAME KEY DETECTED"<<std::endl;
          // Add it to the vector, and finish 
          e.push_back(ent); 
          // Also add to contiguous vector
          ents_contiguous_.push_back(ent);    
          return;  
        }
      }
      // If yes, add in a vector of bodies 
      std::vector<body_holder*> nvent; 
      nvent.push_back(ent); 
      ents_.push_back(nvent);
      ents_contiguous_.push_back(ent); 
      if(ents_.size() > (1<<dimension)){
        refine();
      }
    } // insert
    
    auto begin(){
      return ents_contiguous_.begin();
    }

    auto end(){
      return ents_contiguous_.end();
    }

    auto clear(){
      ents_contiguous_.clear(); 
      ents_.clear();
    }

    void remove(body_holder* ent){
      
      for(auto e = ents_.begin(); e < ents_.end(); ++e){
        //std::vector<body_holder*> elem = *e; 
        auto itr = find(e->begin(), e->end(), ent); 
        if(itr!=e->end()){
          e->erase(itr);
          ents_.erase(e);  
          break; 
        }
      }
      auto itr_contiguous = find(ents_contiguous_.begin(),
        ents_contiguous_.end(),ent); 
      ents_contiguous_.clear(); 
      if(ents_.empty()){
        coarsen();
      } 
    }

    point_t 
    coordinates(
        const std::array<flecsi::point<element_t, dimension>,2>& range) const{
      point_t p;
      branch_id_t bid = id(); 
      bid.coordinates(range,p);
      return p;
    }

    point_t getPosition(){return coordinates_;};
    element_t getMass(){return mass_;};
    element_t getRadius(){return radius_;};
    point_t getBMin(){return bmin_;};
    point_t getBMax(){return bmax_;};
    void setPosition(point_t position){coordinates_ = position;};
    void setMass(element_t mass){mass_ = mass;};
    //void setRadius(element_t radius){radius_ = radius;};
    void setBMax(point_t bmax){bmax_ = bmax;};
    void setBMin(point_t bmin){bmin_ = bmin;};

   private:
    std::vector<std::vector<body_holder*>> ents_;
    std::vector<body_holder*> ents_contiguous_; 
    point_t bmax_;
    point_t bmin_;
  }; // class branch 

  bool should_coarsen(branch* parent){
    return true;
  }

  using branch_t = branch;

}; // class tree_policy

using tree_topology_t = flecsi::topology::tree_topology<tree_policy>;
using body_holder = tree_topology_t::body_holder;
using point_t = tree_topology_t::point_t;
using branch_t = tree_topology_t::branch_t;
using branch_id_t = tree_topology_t::branch_id_t;
using space_vector_t = tree_topology_t::space_vector_t;
using entity_key_t = tree_topology_t::entity_key_t;
using entity_id_t = flecsi::topology::entity_id_t;

std::array<point_t,2> entity_key_t::range_ = {point_t{},point_t{}};

#endif // tree_h

