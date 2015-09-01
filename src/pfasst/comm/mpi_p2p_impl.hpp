#include "pfasst/comm/mpi_p2p.hpp"

#include <exception>
#include <string>
using namespace std;

#include "pfasst/logging.hpp"


MAKE_LOGGABLE(MPI_Status, mpi_status, os)
{
  if (   mpi_status.MPI_TAG == MPI_ANY_TAG
         && mpi_status.MPI_SOURCE == MPI_ANY_SOURCE
         && mpi_status.MPI_ERROR == MPI_SUCCESS) {
    os << "MPI_Status(empty)";
  } else {
    char err_str[MPI_MAX_ERROR_STRING];
    int err_len = 0;
    int err = MPI_Error_string(mpi_status.MPI_ERROR, err_str, &err_len);
    pfasst::comm::check_mpi_error(err);
    os << "MPI_Status(source=" << to_string(mpi_status.MPI_SOURCE) << ", "
    << "tag=" << to_string(mpi_status.MPI_TAG) << ", "
    << "error=" << string(err_str, err_len) << ")";
  }
  return os;
}


namespace pfasst
{
  namespace comm
  {
    string error_from_code(const int err_code)
    {
      char err_str[MPI_MAX_ERROR_STRING];
      int err_len = 0;
      int err = MPI_Error_string(err_code, err_str, &err_len);
      check_mpi_error(err);
      return string(err_str, err_len) + " (code=" + to_string(err_code) + ")";
    }


    MPI_Status MPI_Status_factory()
    {
      MPI_Status stat;
      stat.MPI_ERROR = MPI_SUCCESS;
      stat.MPI_SOURCE = MPI_ANY_SOURCE;
      stat.MPI_TAG = MPI_ANY_TAG;
      return stat;
    }

    void check_mpi_error(const int err_code)
    {
      if (err_code != MPI_SUCCESS) {
        string err_msg = error_from_code(err_code);
        ML_CLOG(ERROR, "COMM_P2P", "MPI encountered an error: " << err_msg);
        throw runtime_error("MPI encountered an error: " + err_msg);
      }
    }


    MpiP2P::MpiP2P(MPI_Comm comm)
      :   _comm(comm)
        , _stati(0)
    {
      log::add_custom_logger("COMM_P2P");

      // get communicator's size and processors rank
      MPI_Comm_size(this->_comm, &(this->_size));
      MPI_Comm_rank(this->_comm, &(this->_rank));

      // get communicator's name (if available)
      int len = -1;
      char buff[MPI_MAX_OBJECT_NAME];
      int err = MPI_Comm_get_name(this->_comm, buff, &len);
      check_mpi_error(err);
      if (len > 0) {
        this->_name = string(buff, len);
      }
    }

    MpiP2P::~MpiP2P()
    {
      this->cleanup();
    }

    size_t MpiP2P::get_size() const
    {
      assert(this->_size > 0);
      return this->_size;
    }

    size_t MpiP2P::get_rank() const
    {
      assert(this->_rank >= 0);
      return this->_rank;
    }

    string MpiP2P::get_name() const
    {
      return this->_name;
    }

    bool MpiP2P::is_first() const
    {
      return (this->get_rank() == this->get_root());
    }

    bool MpiP2P::is_last() const
    {
      return (this->get_rank() == this->get_size() - 1);
    }
    
    void MpiP2P::cleanup()
    {
      for (auto&& req : this->_requests) {
        MPI_Status stat = MPI_Status_factory();
        int err = MPI_Wait(&(req.second), &stat);
        check_mpi_error(err);
        assert(req.second == MPI_REQUEST_NULL);
        this->_requests.erase(req.first);
      }
      assert(this->_requests.size() == 0);
    }

    void MpiP2P::abort(const int& err_code)
    {
      MPI_Abort(this->_comm, err_code);
    }


    void MpiP2P::send(const double* const data, const int count, const int dest_rank, const int tag)
    {
      ML_CLOG(DEBUG, "COMM_P2P", "sending " << count << " double values with tag=" << tag << " to " << dest_rank);
#ifdef NON_CONST_MPI
      int err = MPI_Send(data, count, MPI_DOUBLE, dest_rank, tag, const_cast<MPI_Comm>(this->_comm));
#else
      int err = MPI_Send(data, count, MPI_DOUBLE, dest_rank, tag, this->_comm);
#endif
      check_mpi_error(err);
    }

    void MpiP2P::send_status(const StatusDetail<double>* const data, const int count, const int dest_rank, const int tag)
    {
      assert(pfasst::status_data_type != MPI_DATATYPE_NULL);

      ML_CLOG(DEBUG, "COMM_P2P", "sending " << count << " Status with tag=" << tag << " to " << dest_rank);
#ifdef NON_CONST_MPI
      int err = MPI_Send(data, count, status_data_type, dest_rank, tag, const_cast<MPI_Comm>(this->_comm));
#else
      int err = MPI_Send(data, count, status_data_type, dest_rank, tag, this->_comm);
#endif
      check_mpi_error(err);
    }


    void MpiP2P::isend(const double* const data, const int count, const int dest_rank, const int tag)
    {
      ML_CLOG(DEBUG, "COMM_P2P", "non-blocking send of " << count << " double values with tag=" << tag << " to " << dest_rank);

      auto request_index = make_pair(dest_rank, tag);

      if (this->_requests.find(request_index) != this->_requests.end()) {
        ML_CLOG(WARNING, "COMM_P2P", "request handle does already exists for tag=" << tag << " and destination " << dest_rank
          << " which is still active");
        auto stat = MPI_Status_factory();
        ML_CLOG(DEBUG, "COMM_P2P", "waiting ...");
        int err = MPI_Wait(&(this->_requests[request_index]), &stat);
        check_mpi_error(err);
        ML_CLOG(DEBUG, "COMM_P2P", "waited: " << stat);
      } else {
        MPI_Request this_request = MPI_REQUEST_NULL;
        this->_requests.insert(make_pair(request_index, this_request));
      }

#ifdef NON_CONST_MPI
      int err = MPI_Isend(data, count, MPI_DOUBLE, dest_rank, tag, const_cast<MPI_Comm>(this->_comm), &(this->_requests[request_index]));
#else
      int err = MPI_Isend(data, count, MPI_DOUBLE, dest_rank, tag, this->_comm, &(this->_requests[request_index]));
#endif
      check_mpi_error(err);
    }

    void MpiP2P::isend_status(const StatusDetail<double>* const data, const int count, const int dest_rank, const int tag)
    {
      assert(pfasst::status_data_type != MPI_DATATYPE_NULL);

      ML_CLOG(DEBUG, "COMM_P2P", "non-blocking send of " << count << " Status with tag=" << tag << " to " << dest_rank);

      auto request_index = make_pair(dest_rank, tag);

      if (this->_requests.find(request_index) != this->_requests.end()) {
        ML_CLOG(WARNING, "COMM_P2P", "request handle does already exists for tag=" << tag << " and destination " << dest_rank
        << " which is still active");
        auto stat = MPI_Status_factory();
        ML_CLOG(DEBUG, "COMM_P2P", "waiting ...");
        int err = MPI_Wait(&(this->_requests[request_index]), &stat);
        check_mpi_error(err);
        ML_CLOG(DEBUG, "COMM_P2P", "waited: " << stat);
      } else {
        MPI_Request this_request = MPI_REQUEST_NULL;
        this->_requests.insert(make_pair(request_index, this_request));
      }

#ifdef NON_CONST_MPI
      int err = MPI_Isend(data, count, status_data_type, dest_rank, tag, const_cast<MPI_Comm>(this->_comm), &(this->_requests[request_index]));
#else
      int err = MPI_Isend(data, count, status_data_type, dest_rank, tag, this->_comm, &(this->_requests[request_index]));
#endif
      check_mpi_error(err);
    }


    void MpiP2P::recv(double* data, const int count, const int dest_rank, const int tag)
    {
      this->_stati.push_back(MPI_Status_factory());
      ML_CLOG(DEBUG, "COMM_P2P", "receiving " << count << " double values with tag=" << tag << " from " << dest_rank);
#ifdef NON_CONST_MPI
      int err = MPI_Recv(data, count, MPI_DOUBLE, dest_rank, tag, const_cast<MPI_Comm>(this->_comm), &(this->_stati.back()));
#else
      int err = MPI_Recv(data, count, MPI_DOUBLE, dest_rank, tag, this->_comm, &(this->_stati.back()));
#endif
      check_mpi_error(err);
      ML_CVLOG(1, "COMM_P2P", "--> status: " << this->_stati.back());
    }

    void MpiP2P::recv_status(StatusDetail<double>* data, const int count, const int dest_rank, const int tag)
    {
      assert(pfasst::status_data_type != MPI_DATATYPE_NULL);

      this->_stati.push_back(MPI_Status_factory());
      ML_CLOG(DEBUG, "COMM_P2P", "receiving " << count << " Status with tag=" << tag << " from " << dest_rank);
#ifdef NON_CONST_MPI
      int err = MPI_Recv(data, count, pfasst::status_data_type, dest_rank, tag, const_cast<MPI_Comm>(this->_comm), &(this->_stati.back()));
#else
      int err = MPI_Recv(data, count, pfasst::status_data_type, dest_rank, tag, this->_comm, &(this->_stati.back()));
#endif
      check_mpi_error(err);
      ML_CVLOG(1, "COMM_P2P", "--> status: " << this->_stati.back());
    }


    void MpiP2P::irecv(double* data, const int count, const int src_rank, const int tag)
    {
      ML_CLOG(DEBUG, "COMM_P2P", "non-blocking receive of " << count << " double values with tag=" << tag << " from " << src_rank);

      auto request_index = make_pair(src_rank, tag);

      if (this->_requests.find(request_index) != this->_requests.end()) {
        ML_CLOG(WARNING, "COMM_P2P", "request handle does already exists for tag=" << tag << " and source " << src_rank
                                  << " which is still active");
        auto stat = MPI_Status_factory();
        ML_CLOG(DEBUG, "COMM_P2P", "waiting ...");
        int err = MPI_Wait(&(this->_requests[request_index]), &stat);
        check_mpi_error(err);
        ML_CLOG(DEBUG, "COMM_P2P", "waited: " << stat);
      } else {
        MPI_Request this_request = MPI_REQUEST_NULL;
        this->_requests.insert(make_pair(request_index, this_request));
      }

#ifdef NON_CONST_MPI
      int err = MPI_Irecv(data, count, MPI_DOUBLE, src_rank, tag, const_cast<MPI_Comm>(this->_comm), &(this->_requests[request_index]));
#else
      int err = MPI_Irecv(data, count, MPI_DOUBLE, src_rank, tag, this->_comm, &(this->_requests[request_index]));
#endif
      check_mpi_error(err);
    }

    void MpiP2P::irecv_status(StatusDetail<double>* data, const int count, const int src_rank, const int tag)
    {
      assert(pfasst::status_data_type != MPI_DATATYPE_NULL);

      ML_CLOG(DEBUG, "COMM_P2P", "non-blocking receive of " << count << " Status with tag=" << tag << " from " << src_rank);

      auto request_index = make_pair(src_rank, tag);

      if (this->_requests.find(request_index) != this->_requests.end()) {
        ML_CLOG(WARNING, "COMM_P2P", "request handle does already exists for tag=" << tag << " and source " << src_rank
          << " which is still active");
        auto stat = MPI_Status_factory();
        ML_CLOG(DEBUG, "COMM_P2P", "waiting ...");
        int err = MPI_Wait(&(this->_requests[request_index]), &stat);
        check_mpi_error(err);
        ML_CLOG(DEBUG, "COMM_P2P", "waited: " << stat);
      } else {
        MPI_Request this_request = MPI_REQUEST_NULL;
        this->_requests.insert(make_pair(request_index, this_request));
      }

#ifdef NON_CONST_MPI
      int err = MPI_Irecv(data, count, status_data_type, src_rank, tag, const_cast<MPI_Comm>(this->_comm), &(this->_requests[request_index]));
#else
      int err = MPI_Irecv(data, count, status_data_type, src_rank, tag, this->_comm, &(this->_requests[request_index]));
#endif
      check_mpi_error(err);
    }


    void MpiP2P::bcast(double* data, const int count, const int root_rank)
    {
      ML_CLOG(DEBUG, "COMM_P2P", "broadcasting " << count << " double values from root " << root_rank);
#ifdef NON_CONST_MPI
      int err = MPI_Bcast(data, count, MPI_DOUBLE, root_rank, const_cast<MPI_Comm>(this->_comm));
#else
      int err = MPI_Bcast(data, count, MPI_DOUBLE, root_rank, this->_comm);
#endif
      check_mpi_error(err);
    }
  }
}
