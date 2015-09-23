#ifndef _PFASST__EXAMPLES__ADVEC_DIFF__ADVEC_DIFF_SWEEPER_HPP_
#define _PFASST__EXAMPLES__ADVEC_DIFF__ADVEC_DIFF_SWEEPER_HPP_

#include <memory>
#include <vector>
using namespace std;

#include <pfasst/sweeper/imex.hpp>
#include <pfasst/contrib/fft.hpp>

// I'd really like to have these as static const variable templates but this is only possible since C++14 ... :-(
#define DEFAULT_DIFFUSIVITY 0.02
#define DEFAULT_VELOCITY    1.0


namespace pfasst
{
  namespace examples
  {
    namespace advec_diff
    {
      template<
        class SweeperTrait,
        typename Enabled = void
      >
      class AdvecDiff
        : public IMEX<SweeperTrait, Enabled>
      {
        static_assert(is_same<
                        typename SweeperTrait::encap_type::traits::dim_type,
                        integral_constant<size_t, 1>
                      >::value,
                      "Advection-Diffusion Sweeper requires 1D data structures");

        public:
          typedef          SweeperTrait         traits;
          typedef typename traits::encap_type   encap_type;
          typedef typename traits::time_type    time_type;
          typedef typename traits::spatial_type spatial_type;

          static void init_opts();

        private:
          time_type    _t0;
          spatial_type _nu;
          spatial_type _v;

          pfasst::contrib::FFT<encap_type> _fft;
          vector<complex<spatial_type>>    _ddx;
          vector<complex<spatial_type>>    _lap;

        protected:
          virtual shared_ptr<typename SweeperTrait::encap_type> evaluate_rhs_expl(const typename SweeperTrait::time_type& t,
                                                                                  const shared_ptr<typename SweeperTrait::encap_type> u) override;
          virtual shared_ptr<typename SweeperTrait::encap_type> evaluate_rhs_impl(const typename SweeperTrait::time_type& t,
                                                                                  const shared_ptr<typename SweeperTrait::encap_type> u) override;

          virtual void implicit_solve(shared_ptr<typename SweeperTrait::encap_type> f,
                                      shared_ptr<typename SweeperTrait::encap_type> u,
                                      const typename SweeperTrait::time_type& t,
                                      const typename SweeperTrait::time_type& dt,
                                      const shared_ptr<typename SweeperTrait::encap_type> rhs) override;

          virtual vector<shared_ptr<typename SweeperTrait::encap_type>> compute_error(const typename SweeperTrait::time_type& t);
          virtual vector<shared_ptr<typename SweeperTrait::encap_type>> compute_relative_error(const vector<shared_ptr<typename SweeperTrait::encap_type>>& error, const typename SweeperTrait::time_type& t);

        public:
          explicit AdvecDiff(const size_t& ndofs, const typename SweeperTrait::spatial_type& nu = DEFAULT_DIFFUSIVITY,
                             const typename SweeperTrait::spatial_type& v = DEFAULT_VELOCITY);
          AdvecDiff(const AdvecDiff<SweeperTrait, Enabled>& other) = default;
          AdvecDiff(AdvecDiff<SweeperTrait, Enabled>&& other) = default;
          virtual ~AdvecDiff() = default;
          AdvecDiff<SweeperTrait, Enabled>& operator=(const AdvecDiff<SweeperTrait, Enabled>& other) = default;
          AdvecDiff<SweeperTrait, Enabled>& operator=(AdvecDiff<SweeperTrait, Enabled>&& other) = default;

          virtual void set_options() override;

          virtual shared_ptr<typename SweeperTrait::encap_type> exact(const typename SweeperTrait::time_type& t);

          virtual void post_step() override;

          virtual bool converged(const bool& pre_check = false) override;

          size_t get_num_dofs() const;
      };
    }  // ::pfasst::examples::advec_diff
  }  // ::pfasst::examples
}  // ::pfasst

#include "advec_diff_sweeper_impl.hpp"

#endif  // _PFASST__EXAMPLES__ADVEC_DIFF__ADVEC_DIFF_SWEEPER_HPP_
