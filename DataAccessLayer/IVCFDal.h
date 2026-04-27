#pragma once
#include "DALExport.h"
#include "VCFData.h"

#include <vector>

class DAL_API IVCFDal {
public:

    /*
    * Initialize the database
    * 
    * @param clearDb If true, existing data will be cleared. Default is false.
    */
    virtual void initDb(bool clearDb = false) = 0;


    /*
    * Store batch in a staging table for quick insertion
    * 
    * @param data The batch of VCFData to be stored in the staging table.
    * @throws std::runtime_error if the query fails, with the error message from.
    */
    virtual void storeBatchInStaging(const std::vector<VCFData>& data) = 0;


    /*
    * Finalize records by moving them from staging to main table in sorted order
    * 
    * @throws std::runtime_error if the query fails, with the error message from the underlying storage.
    */
    virtual void finalizeRecords() = 0;


    /*
    * Fetch all variants from the main table, sorted by chrom and pos
    * 
    * @throws std::runtime_error if the query fails, with the error message.
    */
    virtual std::vector<VCFData> fetchAllVariants() = 0;

    virtual ~IVCFDal() = default;
};
