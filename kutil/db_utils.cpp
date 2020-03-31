//
// Created by kh on 3/31/20.
//

#include "kutil.h"

#include <rocksdb/db.h>
#include <rocksdb/slice.h>
#include <rocksdb/options.h>

namespace igen {


int count_keys(str dbpath) {

    using namespace rocksdb;
    Options options;
    // Optimizes total bytes written to disk vs. logical database size (write amplification)
    options.OptimizeUniversalStyleCompaction();
    // create the DB if it's not already present
    options.create_if_missing = true;
    options.keep_log_file_num = options.recycle_log_file_num = 2;
    options.bottommost_compression = options.compression = CompressionType::kZSTD;

    DB *_db_raw_ptr;
    Status s = DB::OpenForReadOnly(options, dbpath, &_db_raw_ptr);
    CHECKF(s.ok(), "Fail to open cachedb (read only): {}, at {}", s.ToString(), dbpath);
    std::unique_ptr<DB> db{_db_raw_ptr};

    str str_est;
    db->GetProperty("rocksdb.estimate-num-keys", &str_est);

    uint64_t cnt = 0;
    rocksdb::ReadOptions read_options;
    read_options.fill_cache = false;
    std::unique_ptr<Iterator> it{db->NewIterator(read_options)};
    for (it->SeekToFirst(); it->Valid(); it->Next()) {
        cnt++;
        if (cnt % 10000 == 0) LOG(INFO, "Progress: {} / est {}", cnt, str_est);
    }
    CHECK(it->status().ok()); // Check for any errors found during the scan
    LOG(WARNING, "Finished counting: {} / est {}", cnt, str_est);

    return 0;
}


}
