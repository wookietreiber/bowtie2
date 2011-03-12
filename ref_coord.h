/*
 * ref_coord.h
 */

#ifndef REF_COORD_H_
#define REF_COORD_H_

#include <stdint.h>
#include <iostream>
#include <algorithm>
#include "assert_helpers.h"

typedef uint64_t TRefId;
typedef int64_t TRefOff;

/**
 * Encapsulates a reference coordinate; i.e. identifiers for (a) a
 * reference sequence, and (b) a 0-based offset into that sequence.
 */
class Coord {

public:

	Coord() { invalidate(); }
	
	Coord(TRefId rf, TRefOff of, bool fw) { init(rf, of, fw); }

	void init(TRefId rf, TRefOff of, bool fw) {
		ref_ = rf;
		off_ = of;
		orient_ = (fw ? 1 : 0);
	}
	
	bool operator==(const Coord& o) const {
		assert(valid());
		assert(o.valid());
		return ref_ == o.ref_ && off_ == o.off_ && fw() == o.fw();
	}

	bool operator<(const Coord& o) const {
		assert(valid());
		assert(o.valid());
		if(ref_ < o.ref_) return true;
		if(ref_ > o.ref_) return false;
		if(off_ < o.off_) return true;
		if(off_ > o.off_) return false;
		if(orient_ < o.orient_) return true;
		return false;
	}
	
	bool operator>(const Coord& o) const {
		assert(valid());
		assert(o.valid());
		if(ref_ > o.ref_) return true;
		if(ref_ < o.ref_) return false;
		if(off_ > o.off_) return true;
		if(off_ < o.off_) return false;
		if(orient_ > o.orient_) return true;
		return false;
	}
	
	/**
	 * Make this coord invalid.
	 */
	void invalidate() {
		ref_ = std::numeric_limits<TRefId>::max();
		off_ = std::numeric_limits<TRefOff>::max();
		orient_ = -1;
	}
	
	/**
	 * Return true iff this Coord is valid (i.e. ref and off have both
	 * been set since the last call to invalidate()).
	 */
	bool valid() const {
		if(ref_ != std::numeric_limits<TRefId>::max() &&
		   off_ != std::numeric_limits<TRefOff>::max())
		{
			assert(orient_ == 0 || orient_ == 1);
			return true;
		}
		return false;
	}
	
	/**
	 * Get orientation of the Coord.
	 */
	bool fw() const {
		assert(valid());
		assert(orient_ == 0 || orient_ == 1);
		return orient_ == 1;
	}
	
	/**
	 * Check that coord is internally consistent.
	 */
	bool repOk() const {
		if(ref_ != std::numeric_limits<TRefId>::max() &&
		   off_ != std::numeric_limits<TRefOff>::max())
		{
			assert(orient_ == 0 || orient_ == 1);
		}
		return true;
	}
	
	/**
	 * Check whether an interval defined by this coord and having
	 * length 'len' is contained within an interval defined by
	 * 'inbegin' and 'inend'.
	 */
	bool within(int64_t len, int64_t inbegin, int64_t inend) const {
		return off_ >= inbegin && off_ + len <= inend;
	}
	
	TRefId  ref() const { return ref_; }
	TRefOff off() const { return off_; }

protected:

	TRefId  ref_;    // which reference?
	TRefOff off_;    // 0-based offset into reference
	int     orient_; // true -> Watson strand
};

std::ostream& operator<<(std::ostream& out, const Coord& c);

/**
 * Encapsulates a reference interval.
 */
class Interval {

public:
	
	Interval() { invalidate(); }
	
	Interval(const Coord& upstream, TRefOff len) {
		init(upstream, len);
	}

	Interval(TRefId id1, TRefId id2, TRefOff off1, TRefOff off2) {
		assert_eq(id1, id2);
		TRefOff upstream = std::min(off1, off2);
		TRefOff dnstream = std::max(off1, off2);
		Coord c(id1, upstream, true);
		init(c, dnstream - upstream);
	}

	void init(const Coord& upstream, TRefOff len) {
		upstream_ = upstream;
		len_ = len;
	}

	/**
	 * Make this coord invalid.
	 */
	void invalidate() {
		upstream_.invalidate();
		len_ = 0;
	}
	
	/**
	 * Return true iff this Interval is valid.
	 */
	bool valid() const {
		if(upstream_.valid()) {
			assert_gt(len_, 0);
			return true;
		} else {
			return false;
		}
	}
	
	/**
	 * Return true iff this Interval is equal to the given Interval,
	 * i.e. if they cover the same set of positions.
	 */
	bool operator==(const Interval& o) const {
		return upstream_ == o.upstream_ &&
		       len_ == o.len_;
	}

	/**
	 * Return true iff this Interval is less than the given Interval.
	 * One interval is less than another if its upstream location is
	 * prior to the other's or, if their upstream locations are equal,
	 * if its length is less than the other's.
	 */
	bool operator<(const Interval& o) const {
		if(upstream_ < o.upstream_) return true;
		if(upstream_ > o.upstream_) return false;
		if(len_ < o.len_) return true;
		return false;
	}

	/**
	 * Return true iff this Interval is greater than than the given
	 * Interval.  One interval is greater than another if its upstream
	 * location is after the other's or, if their upstream locations
	 * are equal, if its length is greater than the other's.
	 */
	bool operator>(const Interval& o) const {
		if(upstream_ > o.upstream_) return true;
		if(upstream_ < o.upstream_) return false;
		if(len_ > o.len_) return true;
		return false;
	}
	
	const Coord&  upstream() const { return upstream_; }
	TRefOff       len()      const { return len_;      }

protected:

	Coord   upstream_;
	TRefOff len_;
};

std::ostream& operator<<(std::ostream& out, const Interval& c);

#endif /*ndef REF_COORD_H_*/
