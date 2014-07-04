/*
 * Base controller (see also SDC, MLSDC, and PFASST controllers).
 */

#ifndef _PFASST_CONTROLLER_HPP_
#define _PFASST_CONTROLLER_HPP_

#include "interfaces.hpp"

namespace pfasst
{

  /**
   * base SDC/MLSDC/PFASST controller.
   * @tparam time time precision
   *     defaults to pfasst::time_precision
   */
  template<typename time = time_precision>
  class Controller
  {
    protected:
      deque<shared_ptr<ISweeper<time>>>  levels;
      deque<shared_ptr<ITransfer<time>>> transfer;

      size_t nsteps, niters;
      time   dt;

    public:
      void setup()
      {
        for (auto l = coarsest(); l <= finest(); ++l) {
          l.current()->setup();
        }
      }

      void set_duration(time dt, size_t nsteps, size_t niters)
      {
        this->dt = dt;
        this->nsteps = nsteps;
        this->niters = niters;
      }

      void add_level(ISweeper<time>* swpr, ITransfer<time>* trnsfr = NULL, bool coarse = true)
      {
        if (coarse) {
          levels.push_front(shared_ptr<ISweeper<time>>(swpr));
          transfer.push_front(shared_ptr<ITransfer<time>>(trnsfr));
        } else {
          levels.push_back(shared_ptr<ISweeper<time>>(swpr));
          transfer.push_back(shared_ptr<ITransfer<time>>(trnsfr));
        }
      }

      template<typename R = ISweeper<time>> R* get_level(size_t level)
      {
        return dynamic_cast<R*>(levels[level].get());
      }

      template<typename R = ITransfer<time>> R* get_transfer(size_t level)
      {
        return dynamic_cast<R*>(transfer[level].get());
      }

      size_t nlevels()
      {
        return levels.size();
      }

      /**
       * level (MLSDC/PFASST) iterator.
       * 
       * This iterator is used to walk through the MLSDC/PFASST hierarchy of sweepers.
       * It keeps track of the _current_ level, and has convenience routines to return the 
       * LevelIter::current(), LevelIter::fine() (i.e. `current+1`), and LevelIter::coarse()
       * (`current-1`) sweepers.
       */
      class LevelIter
      {
          Controller* ts;

        public:
          size_t level;

          LevelIter(size_t level, Controller* ts) : ts(ts), level(level) {}

          template<typename R = ISweeper<time>> R* current()
          {
            return ts->get_level<R>(level);
          }
          template<typename R = ISweeper<time>> R* fine()
          {
            return ts->get_level<R>(level + 1);
          }
          template<typename R = ISweeper<time>> R* coarse()
          {
            return ts->get_level<R>(level - 1);
          }
          template<typename R = ITransfer<time>> R* transfer()
          {
            return ts->get_transfer<R>(level);
          }

          ISweeper<time>* operator*() { return current(); }
          bool operator==(LevelIter i) { return level == i.level; }
          bool operator!=(LevelIter i) { return level != i.level; }
          bool operator<=(LevelIter i) { return level <= i.level; }
          bool operator>=(LevelIter i) { return level >= i.level; }
          bool operator< (LevelIter i) { return level <  i.level; }
          bool operator> (LevelIter i) { return level >  i.level; }
          LevelIter operator- (size_t i) { return LevelIter(level - i, ts); }
          LevelIter operator+ (size_t i) { return LevelIter(level + i, ts); }
          void operator++() { level++; }
          void operator--() { level--; }
      };

      LevelIter finest()   { return LevelIter(nlevels() - 1, this); }
      LevelIter coarsest() { return LevelIter(0, this); }

  };

}

#endif
