/*
 *  pe.h
 *
 * A class encapsulating a paired-end policy and routines for
 * identifying intervals according to the policy.  For instance,
 * contains a routine that, given a policy and details about a match
 * for one mate, returns details about where to search for the other
 * mate.
 */

#ifndef PE_H_
#define PE_H_

#include <iostream>
#include <stdint.h>

// In description below "To the left" = "Upstream of w/r/t the Watson strand"

// The 4 possible policies describing how mates 1 and 2 should be
// oriented with respect to the reference genome and each other
enum {
	// (fw) Both mates from Watson with 1 to the left, or
	// (rc) Both mates from Crick with 2 to the left
	PE_POLICY_FF = 1,

	// (fw) Both mates from Crick with 1 to the left, or
	// (rc) Both mates from Watson with 2 to the left
	PE_POLICY_RR,
	
	// (fw) Mate 1 from Watson and mate 2 from Crick with 1 to the left, or
	// (rc) Mate 2 from Watson and mate 1 from Crick with 2 to the left
	PE_POLICY_FR,
	
	// (fw) Mate 1 from Crick and mate 2 from Watson with 1 to the left, or
	// (rc) Mate 2 from Crick and mate 1 from Watson with 2 to the left
	PE_POLICY_RF
};

// Various distinct ways that the mates might align with respect to
// each other in a concordant alignment.  We distinguish between them
// because in some cases a user may want to consider some of these
// categories to be discordant, even if the alignment otherwise
// conforms to the paired-end policy.

enum {
	// Describes a paired-end alignment where the mates
	// straightforwardly conform to the paired-end policy without any
	// overlap between the mates
	PE_ALS_NORMAL = 1,

	// Describes a paired-end alignment where the mate overlap, but
	// neither contains the other and they do not dovetail, but the
	// alignment conforms to the paired-end policy
	PE_ALS_OVERLAP,
	
	// Describes a paired-end alignment where the mates conform to the
	// paired-end policy, but one mate strictly contains the other but
	// they don't dovetail.  We distinguish this from a "normal"
	// concordant alignment because some users may wish to categorize
	// such an alignment as discordant.
	PE_ALS_CONTAIN,
	
	// Describes a paired-end alignment where the mates conform to the
	// paired-end policy, but mates "fall off" each other.  E.g. if the
	// policy is FR and any of these happen:
	// 1:     >>>>>   >>>>>
	// 2:  <<<<<<    <<<<<<
	// And the overall extent is consistent with the minimum fragment
	// length, this is a dovetail alignment.  We distinguish this from
	// a "normal" concordant alignment because some users may wish to
	// categorize such an alignment as discordant.
	PE_ALS_DOVETAIL,
	
	// The mates are clearly discordant, owing to their orientations
	// and/or implied fragment length
	PE_ALS_DISCORD
};

/**
 * Return true iff the orientations and relative positions of mates 1
 * and 2 are compatible with the given PE_POLICY.
 */
static inline bool pePolicyCompat(
	int policy,   // PE_POLICY
	bool oneLeft, // true iff mate 1 is to the left of mate 2
	bool oneWat,  // true iff mate 1 aligned to Watson strand
	bool twoWat)  // true iff mate 2 aligned to Watson strand
{
	switch(policy) {
		case PE_POLICY_FF:
			return oneWat == twoWat && oneWat == oneLeft;
		case PE_POLICY_RR:
			return oneWat == twoWat && oneWat != oneLeft;
		case PE_POLICY_FR:
			return oneWat != twoWat && oneWat == oneLeft;
		case PE_POLICY_RF:
			return oneWat != twoWat && oneWat != oneLeft;
		default: {
			std::cerr << "Bad PE_POLICY: " << policy << std::endl;
			throw 1;
		}
	}
	throw 1;
}

/**
 * Given that the given mate aligns in the given orientation, return
 * true iff the other mate must appear "to the right" of the given mate
 * in order for the alignment to be concordant.
 */
static inline void pePolicyMateDir(
	int   policy,// in: PE_POLICY
	bool  is1,   // in: true iff mate 1 is the one that already aligned
	bool  fw,    // in: true iff already-aligned mate aligned to Watson
	bool& left,  // out: set =true iff other mate must be to the left
	bool& mfw)   // out: set =true iff other mate must align to watson
{
	switch(policy) {
		case PE_POLICY_FF: {
			left = (is1 != fw);
			mfw = fw;
			break;
		}
		case PE_POLICY_RR: {
			left = (is1 == fw);
			mfw = fw;
			break;
		}
		case PE_POLICY_FR: {
			left = !fw;
			mfw = !fw;
			break;
		}
		case PE_POLICY_RF: {
			left = fw;
			mfw = !fw;
			break;
		}
		default: {
			std::cerr << "Error: No such PE_POLICY: " << policy << std::endl;
			throw 1;
		}
	}
	return;
}

/**
 * Encapsulates paired-end alignment parameters.
 */
class PairedEndPolicy {

public:

	PairedEndPolicy() { reset(); }
	
	PairedEndPolicy(
		int pol,
		uint32_t maxfrag,
		uint32_t minfrag,
		bool local,
		bool dovetailOk,
		bool containOk,
		bool olapOk,
		bool expandToFit)
	{
		init(
			pol,
			maxfrag,
			minfrag,
			local,
			dovetailOk,
			containOk,
			olapOk,
			expandToFit);
	}

	/** 
	 * Initialize with nonsense values.
	 */
	void reset() {
		init(-1, 0xffffffff, 0xffffffff, false, false, false, false, false);
	}

	/**
	 * Initialize given policy, maximum & minimum fragment lengths.
	 */
	void init(
		int pol,
		uint32_t maxfrag,
		uint32_t minfrag,
		bool local,
		bool dovetailOk,
		bool containOk,
		bool olapOk,
		bool expandToFit)
	{
		pol_         = pol;
		maxfrag_     = maxfrag;
		minfrag_     = minfrag;
		local_       = local;
		dovetailOk_  = dovetailOk;
		containOk_   = containOk;
		olapOk_      = olapOk;
		expandToFit_ = expandToFit;
	}

	/**
	 * Given details about how one mate aligns, and some details about
	 * the reference sequence it aligned to, calculate a window and
	 * orientation s.t. the alignment for the pair will be concordant
	 * if the other mate aligns with that orientation in that window.
	 */
	bool otherMate(
		bool     is1,       // true -> mate 1 aligned and we're looking
							// for 2, false -> vice versa
		bool     fw,        // orientation of aligned mate
		int64_t  off,       // offset into the reference sequence
		uint32_t reflen,    // length of reference sequence aligned to
		uint32_t len1,      // length of mate 1
		uint32_t len2,      // length of mate 2
		int      maxgaps,   // maximum number of gaps permitted in the
							// alignment of the opposite mate
		int      maxohang,  // maximum overhang of dynamic programming
							// region off end of reference
		bool&    oleft,     // whether to look to the left for opposite mate
		int64_t& oleftoff,  // offset of leftmost character to include in
							// dyn prog problem looking for opposite mate
		int64_t& orightoff, // offset of rightmost character to include in
							// dyn prog problem looking for opposite mate
		bool&    ofw)       // whether to look for opposite mate's forward
		const;              // or reverse-comp representation

	/**
	 * Return a PE_TYPE flag indicating, given a PE_POLICY and coordinates
	 * for a paired-end alignment, what type of alignment it is, i.e.,
	 * whether it's:
	 *
	 * 1. Straightforwardly concordant
	 * 2. Mates dovetail (one extends beyond the end of the other)
	 * 3. One mate contains the other but they don't dovetail
	 * 4. One mate overlaps the other but neither contains the other and
	 *    they don't dovetail
	 * 5. Discordant
	 */
	int peClassifyPair(
		int64_t  off1,   // offset of mate 1
		uint32_t len1,   // length of mate 1
		bool     fw1,    // whether mate 1 aligned to Watson
		int64_t  off2,   // offset of mate 2
		uint32_t len2,   // length of mate 2
		bool     fw2)    // whether mate 2 aligned to Watson
		const;

	int      policy()     const { return pol_;     }
	uint32_t maxFragLen() const { return maxfrag_; }
	uint32_t minFragLen() const { return minfrag_; }

protected:

	// Use local alignment to search for the opposite mate, rather than
	// a type of alignment that requires the read to align end-to-end
	bool local_;

	// Policy governing how mates should be oriented with respect to
	// each other and the reference genome
	int pol_;
	
	// true iff settings are such that dovetailed mates should be
	// considered concordant.
	bool dovetailOk_;

	// true iff paired-end alignments where one mate's alignment is
	// strictly contained within the other's should be considered
	// concordant
	bool containOk_;

	// true iff paired-end alignments where one mate's alignment
	// overlaps the other's should be considered concordant
	bool olapOk_;
	
	// What to do when a mate length is > maxfrag_?  If expandToFit_ is
	// true, we temporarily increase maxfrag_ to equal the mate length.
	// Otherwise we say that any paired-end alignment involving the
	// long mate is discordant.
	bool expandToFit_;
	
	// Maximum fragment size to consider
	uint32_t maxfrag_;

	// Minimum fragment size to consider
	uint32_t minfrag_;
};

#endif /*ndef PE_H_*/
