#ifndef _PFASST__SWEEPER__IMEX_HPP_
#define _PFASST__SWEEPER__IMEX_HPP_

#include "pfasst/sweeper/interface.hpp"


namespace pfasst
{
  template<
    class SweeperTrait,
    typename Enabled = void
  >
  class IMEX
    : public Sweeper<SweeperTrait, Enabled>
  {
    public:
      typedef          SweeperTrait                 sweeper_traits;
      typedef typename sweeper_traits::encap_type   encap_type;
      typedef typename sweeper_traits::time_type    time_type;
      typedef typename sweeper_traits::spacial_type spacial_type;

    protected:
      vector<shared_ptr<encap_type>> _q_integrals;
      vector<shared_ptr<encap_type>> _expl_rhs;
      vector<shared_ptr<encap_type>> _impl_rhs;

      size_t num_expl_f_evals;
      size_t num_impl_f_evals;

      virtual void integrate_end_state(const typename SweeperTrait::time_type& dt);
      virtual void compute_residuals();

      virtual shared_ptr<typename SweeperTrait::encap_type> evaluate_rhs_expl(const typename SweeperTrait::time_type& t,
                                                                              const shared_ptr<typename SweeperTrait::encap_type> u);
      virtual shared_ptr<typename SweeperTrait::encap_type> evaluate_rhs_impl(const typename SweeperTrait::time_type& t,
                                                                              const shared_ptr<typename SweeperTrait::encap_type> u);

      virtual void implicit_solve(shared_ptr<typename SweeperTrait::encap_type> f,
                                  shared_ptr<typename SweeperTrait::encap_type> u,
                                  const typename SweeperTrait::time_type& t,
                                  const typename SweeperTrait::time_type& dt,
                                  const shared_ptr<typename SweeperTrait::encap_type> rhs);

    public:
      explicit IMEX();
      IMEX(const IMEX<SweeperTrait, Enabled>& other) = default;
      IMEX(IMEX<SweeperTrait, Enabled>&& other) = default;
      virtual ~IMEX() = default;
      IMEX<SweeperTrait, Enabled>& operator=(const IMEX<SweeperTrait, Enabled>& other) = default;
      IMEX<SweeperTrait, Enabled>& operator=(IMEX<SweeperTrait, Enabled>&& other) = default;

      virtual void setup();

      virtual void pre_predict();
      virtual void predict();
      virtual void post_predict();

      virtual void pre_sweep();
      virtual void sweep();
      virtual void post_sweep();

      virtual void advance();
      virtual void reevaluate(const bool initial_only=false);
  };
}  // ::pfasst

#include "pfasst/sweeper/imex_impl.hpp"

#endif  // _PFASST__SWEEPER__IMEX_HPP_