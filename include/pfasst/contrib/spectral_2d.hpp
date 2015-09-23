#ifndef _PFASST__TRANSFER__SPECTRAL_2D_HPP_
#define _PFASST__TRANSFER__SPECTRAL_2D_HPP_

#include "pfasst/transfer/polynomial.hpp"

#include <memory>
#include <vector>
using namespace std;

#include "pfasst/quadrature.hpp"
#include "pfasst/contrib/fft.hpp"


namespace pfasst
{
  namespace contrib
  {
    template<
      class TransferTraits,
      typename Enabled = void
    >
    class Spectral2DTransfer
            : public PolynomialTransfer<TransferTraits, Enabled>
    {
      public:
        typedef TransferTraits traits;

        typedef typename traits::coarse_encap_traits coarse_encap_traits;
        typedef typename traits::coarse_encap_type coarse_encap_type;
        typedef typename traits::coarse_time_type coarse_time_type;
        typedef typename traits::coarse_spatial_type coarse_spatial_type;

        typedef typename traits::fine_encap_traits fine_encap_traits;
        typedef typename traits::fine_encap_type fine_encap_type;
        typedef typename traits::fine_time_type fine_time_type;
        typedef typename traits::fine_spatial_type fine_spatial_type;

      protected:
        pfasst::contrib::FFT<fine_encap_type> fft;

      public:
        Spectral2DTransfer() = default;

        Spectral2DTransfer(const Spectral2DTransfer<TransferTraits, Enabled> &other) = default;

        Spectral2DTransfer(Spectral2DTransfer<TransferTraits, Enabled> &&other) = default;

        virtual ~Spectral2DTransfer() = default;

        Spectral2DTransfer<TransferTraits, Enabled>& operator=(const Spectral2DTransfer<TransferTraits, Enabled> &other) = default;

        Spectral2DTransfer<TransferTraits, Enabled>& operator=(Spectral2DTransfer<TransferTraits, Enabled> &&other) = default;

        virtual void interpolate_data(const shared_ptr<typename TransferTraits::coarse_encap_type> coarse,
                                      shared_ptr<typename TransferTraits::fine_encap_type> fine);

        virtual void restrict_data(const shared_ptr<typename TransferTraits::fine_encap_type> fine,
                                   shared_ptr<typename TransferTraits::coarse_encap_type> coarse);
    };
  }  // ::pfasst::contrib
}  // ::pfasst

#include "pfasst/contrib/spectral_2d_impl.hpp"

#endif  // _PFASST__TRANSFER__SPECTRAL_2D_HPP_
