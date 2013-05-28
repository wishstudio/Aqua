#pragma once

#include "PlatformTypes.h"

template<typename SlotType>
class _HashBase
{
protected:
	static const int MINIMUM_CAPACITY = 64;
	static const int MAX_RATIO_NUMERATOR = 3;
	static const int MAX_RATIO_DENOMINATOR = 4;
	/* Max usage before rehash: 3 / 4 = 75% */
	uint32 currentCapacity, currentUsage;
	SlotType *hashSlots;

	/* Ensure usage ratio is acceptable, rehash if necessary */
	inline void ensureCapacity()
	{
		if (currentUsage * MAX_RATIO_DENOMINATOR < currentCapacity * MAX_RATIO_NUMERATOR)
			return;
		uint32 oldCapacity = currentCapacity;
		SlotType *oldSlots = hashSlots;
		currentCapacity = (currentCapacity == 0)? MINIMUM_CAPACITY: currentCapacity * 2;
		hashSlots = (SlotType *) calloc(currentCapacity, sizeof(SlotType));
		if (oldCapacity > 0)
		{
			for (uint32 i = 0; i < oldCapacity; i++)
				if (oldSlots[i].value)
					oldSlots[i].insertTo(this);
		}
		free(oldSlots);
	}

public:
	inline _HashBase(): currentCapacity(0), currentUsage(0)
	{
		ensureCapacity();
	}
};

template<typename KeyType, typename ValueType> class PointerHash1;
template<typename KeyType, typename ValueType>
struct _PointerHash1Slot
{
	KeyType *key;
	ValueType *value;

	inline void insertTo(_HashBase<_PointerHash1Slot<KeyType, ValueType>> *hash)
	{
		((PointerHash1<KeyType, ValueType> *) hash)->insert(key, value);
	}
};
template<typename KeyType, typename ValueType>
class PointerHash1: public _HashBase<_PointerHash1Slot<KeyType, ValueType>>
{
private:
	inline nativeuint hash(KeyType *key)
	{
		return (nativeuint) key;
	}

public:
	void insert(KeyType *key, ValueType *value)
	{
		ensureCapacity();
		uint32 h = hash(key) % currentCapacity;
		while (hashSlots[h].value)
		{
			h++;
			if (h == currentCapacity)
				h = 0;
		}
		hashSlots[h].key = key;
		hashSlots[h].value = value;
		currentUsage++;
	}

	ValueType *find(KeyType *key)
	{
		uint32 h = hash(key) % currentCapacity;
		for (;;)
		{
			if (!hashSlots[h].value)
				return NULL;
			if (hashSlots[h].key == key)
				return hashSlots[h].value;
			h++;
			if (h == currentCapacity)
				h = 0;
		}
	}
};

template<typename KeyType1, typename KeyType2, typename ValueType> class PointerHash2;
template<typename KeyType1, typename KeyType2, typename ValueType>
struct _PointerHash2Slot
{
	KeyType1 *key1;
	KeyType2 *key2;
	ValueType *value;

	inline void insertTo(_HashBase<_PointerHash2Slot<KeyType1, KeyType2, ValueType>> *hash)
	{
		((PointerHash2<KeyType1, KeyType2, ValueType> *) hash)->insert(key1, key2, value);
	}
};
template<typename KeyType1, typename KeyType2, typename ValueType>
class PointerHash2: public _HashBase<_PointerHash2Slot<KeyType1, KeyType2, ValueType>>
{
private:
	inline nativeuint hash(KeyType1 *key1, KeyType2 *key2)
	{
		return (nativeuint) key1 + ((nativeuint) key2 >> 8);
	}

public:
	void insert(KeyType1 *key1, KeyType2 *key2, ValueType *value)
	{
		ensureCapacity();
		uint32 h = hash(key1, key2) % currentCapacity;
		while (hashSlots[h].value)
		{
			h++;
			if (h == currentCapacity)
				h = 0;
		}
		hashSlots[h].key1 = key1;
		hashSlots[h].key2 = key2;
		hashSlots[h].value = value;
		currentUsage++;
	}

	ValueType *find(KeyType1 *key1, KeyType2 *key2)
	{
		uint32 h = hash(key1, key2) % currentCapacity;
		for (;;)
		{
			if (!hashSlots[h].value)
				return NULL;
			if (hashSlots[h].key1 == key1 && hashSlots[h].key2 == key2)
				return hashSlots[h].value;
			h++;
			if (h == currentCapacity)
				h = 0;
		}
	}
};

template<typename KeyType1, typename KeyType2, typename KeyType3, typename ValueType> class PointerHash3;
template<typename KeyType1, typename KeyType2, typename KeyType3, typename ValueType>
struct _PointerHash3Slot
{
	KeyType1 *key1;
	KeyType2 *key2;
	KeyType3 *key3;
	ValueType *value;

	inline void insertTo(_HashBase<_PointerHash3Slot<KeyType1, KeyType2, KeyType3, ValueType>> *hash)
	{
		((PointerHash3<KeyType1, KeyType2, KeyType3, ValueType> *) hash)->insert(key1, key2, key3, value);
	}
};
template<typename KeyType1, typename KeyType2, typename KeyType3, typename ValueType>
class PointerHash3: public _HashBase<_PointerHash3Slot<KeyType1, KeyType2, KeyType3, ValueType>>
{
private:
	inline nativeuint hash(KeyType1 *key1, KeyType2 *key2, KeyType3 *key3)
	{
		return (nativeuint) key1 + ((nativeuint) key2 >> 8) + ((nativeuint) key3 >> 16);
	}

public:
	void insert(KeyType1 *key1, KeyType2 *key2, KeyType3 *key3, ValueType *value)
	{
		ensureCapacity();
		uint32 h = hash(key1, key2, key3) % currentCapacity;
		while (hashSlots[h].value)
		{
			h++;
			if (h == currentCapacity)
				h = 0;
		}
		hashSlots[h].key1 = key1;
		hashSlots[h].key2 = key2;
		hashSlots[h].key3 = key3;
		hashSlots[h].value = value;
		currentUsage++;
	};

	ValueType *find(KeyType1 *key1, KeyType2 *key2, KeyType3 *key3)
	{
		uint32 h = hash(key1, key2, key3) % currentCapacity;
		for (;;)
		{
			if (!hashSlots[h].value)
				return NULL;
			if (hashSlots[h].key1 == key1 && hashSlots[h].key2 == key2 && hashSlots[h].key3 == key3)
				return hashSlots[h].value;
			h++;
			if (h == currentCapacity)
				h = 0;
		}
	}
};

/* General hash used for interning
 * Needed static functions:
 * static uint32 KeyType::hash(KeyType *key);
 * static uint32 KeyType::isEqual(KeyType *left, KeyType *right);
 */
template<typename ValueType> class InternHash;
template<typename ValueType>
struct _InternHashSlot
{
	ValueType *value;

	inline void insertTo(_HashBase<_InternHashSlot<ValueType>> *hash)
	{
		((InternHash<ValueType> *) hash)->findOrCreate(value);
	}
};
template<typename ValueType>
class InternHash: public _HashBase<_InternHashSlot<ValueType>>
{
public:
	typedef ValueType * (* CreateFunction) (ValueType *value);
	
	static ValueType *_defaultCreateFunction(ValueType *value)
	{
		return value;
	}

	ValueType *findOrCreate(ValueType *value, CreateFunction create = _defaultCreateFunction)
	{
		ensureCapacity();
		uint32 h = ValueType::hash(value) % currentCapacity;
		for (;;)
		{
			if (!hashSlots[h].value)
			{
				hashSlots[h].value = create(value);
				currentUsage++;
				return hashSlots[h].value;
			}
			if (ValueType::isEqual(value, hashSlots[h].value))
				return hashSlots[h].value;
			h++;
			if (h == currentCapacity)
				h = 0;
		}
	}
};
