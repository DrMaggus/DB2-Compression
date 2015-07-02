#pragma once

#include <core/compressed_column.hpp>
#include <vector>

namespace CoGaDB {

	/*
	
	This struct "Twee" [ dutch for 'two' ] holds for each different value in the
	column, which is to be compressed, the count how often it appears in the
	column (in a block) and it holds the value itself.

	For easier load and store it also has a function serialize.
	
	*/

	template<typename T>
	struct Twee {
		unsigned int count = 0;
		T value;

		Twee(){ count = 1; }

		template<typename T>
		Twee(int c, T val){ count = c; value = val; }

		template<class Archive>
		void serialize(Archive & ar, const unsigned int version)
		{
			ar & count;
			ar & value;  
		}
	};




	template<typename T>
	class RunLengthCompressionColumn : public  CoGaDB::CompressedColumn<T>
	{
	public:
		RunLengthCompressionColumn<T>(const std::string &name, AttributeType db_type);
		~RunLengthCompressionColumn<T>();

		virtual bool insert(const boost::any& new_Value);
		virtual bool insert(const T& new_value);

		virtual bool update(TID tid, const boost::any& new_value);
		virtual bool update(PositionListPtr tid, const boost::any& new_value);

		virtual bool remove(TID tid);
		virtual bool remove(PositionListPtr tid);
		virtual bool clearContent();

		virtual const boost::any get(TID tid);
		virtual void print() const throw();
		virtual size_t size() const throw();
		virtual unsigned int getSizeinBytes() const throw();

		virtual const ColumnPtr copy() const;

		virtual bool store(const std::string& path);
		virtual bool load(const std::string& path);

		virtual T& operator[](const int index);

		template<class Archive>
		void serialize(Archive & ar, const unsigned int version)
		{
			ar & this->values;
			ar & this->elemNum;
		}

	private:

		/*
		
		A std::vector represents the column. It holds a 'Twee<T>' for each value.

		We keep track of how many values we have in our column with 'elemNum',
		so we don't have to calculate everytime.
		
		*/

		int TIDtoCompressedIndex(TID tid);
		std::vector<Twee<T>> values;
		unsigned int elemNum;
	};

	template<class T>
	RunLengthCompressionColumn<T>::RunLengthCompressionColumn(const std::string &name, AttributeType db_type) : CompressedColumn<T>(name, db_type){
		elemNum = 0;
	}

	template<class T>
	RunLengthCompressionColumn<T>::~RunLengthCompressionColumn(){

	}

	/*
	To match the "original" tid of a value with the respective one in the compressed column.
	It is necessary to compute the respective position in the values-vector.
	*/
	template<class T>
	int RunLengthCompressionColumn<T>::TIDtoCompressedIndex(TID tid) {
		unsigned int tid_ = 0;
		for (unsigned int i = 0; i < this->values.size(); i++) {
			tid_ += this->values[i].count;
			if (tid_ > tid)
				return i;
		}
		return -1; //if tid is out of bound -> return -1 => exception is thrown by std::vector
	}

	/*
	
	To insert a new value we only have to check whether the last inserted value is
	equal to the new value. If so we just increase the count of the value.
	Otherwise we just add the new value to the vector.
	
	*/
	template<class T>
	bool RunLengthCompressionColumn<T>::insert(const T &new_value){
			if (this->values.empty() || this->values[this->values.size() - 1].value != new_value)
			{
				this->values.push_back(Twee<T>(1, new_value));
				this->elemNum++;
				return true;
			}
			else {
				this->values[this->values.size() - 1].count++;
				this->elemNum++;
				return true;
			}
	}

	//uses the other insert function
	template<class T>
	bool RunLengthCompressionColumn<T>::insert(const boost::any& new_value){
		if (new_value.empty())
			return false;
		if (typeid(T) == new_value.type()){
			T value = boost::any_cast<T>(new_value);
			this->insert(value);
			return true;
		}
		return false;
	}

	template<class T>
	const boost::any RunLengthCompressionColumn<T>::get(TID tid){
		if (tid < this->elemNum)
			return boost::any(this->values[this->TIDtoCompressedIndex(tid)].value);

		std::cout << "fatal Error!!! Invalid TID!!! Attribute: " << this->name_ << " TID: " << tid << std::endl;
		return boost::any();
	}

	template<class T>
	void RunLengthCompressionColumn<T>::print() const throw(){
		std::cout << "| " << this->name_ << " |" << std::endl;
		std::cout << "________________________" << std::endl;
		for (unsigned int i = 0; i < values.size(); i++){
			std::cout << "| " << values[i].count << " | " << values[i].value << " |" << std::endl;
		}

	}
	template<class T>
	size_t RunLengthCompressionColumn<T>::size() const throw(){
		return this->elemNum;
	}
	template<class T>
	const ColumnPtr RunLengthCompressionColumn<T>::copy() const{
		return ColumnPtr(new RunLengthCompressionColumn(*this)); //no dynamic memory allocations or pointer variables
	}

	template<class T>
	bool RunLengthCompressionColumn<T>::update(TID tid, const boost::any& obj){

		//// Find index in compressed column and respective offset of tid in this block
		if (typeid(T) == obj.type()) {
			int tid_ = 0, index, offset;
			for (unsigned int i = 0; i < this->values.size(); i++) {
				for (unsigned int j = 0; j < this->values[i].count; j++) {
					if (tid == tid_) {
						offset = j;
						index = i;
					}
					tid_++;
				}
			}

			std::vector<T> left; // the "Twee-Block" of the values "left" (before) the updated "Twee-Block"
			std::vector<T> right; // the "Twee-Block" of the values "right" (after) the updated "Twee-Block"

			if (index != 0)
				left.insert(left.begin(), this->values[index - 1].count, this->values[index - 1].value);
			if (index != this->values.size() - 1)
				right.insert(right.begin(), this->values[index + 1].count, this->values[index + 1].value);


			//Merging left, right and the updated TweeBlock to a list (in correct order)
			std::vector<T> list(this->values[index].count, this->values[index].value);
			list[offset] = boost::any_cast<T>(obj);

			list.insert(list.begin(), left.begin(), left.end());
			list.insert(list.end(), right.begin(), right.end());

			//Check whether we are at the beginning or end of a column
			//=> so we have no Twee-Block before the updated Block or after it.
			if (!left.empty())
				this->values[index - 1].count = 0;
			if (!right.empty())
				this->values[index + 1].count = 0;
			this->values[index].count = 0;


			//"Before","After" and the updated Block are marked as 'count == 0'
			//They need to be deleted to afterwards  put the std::vector list at the right position
			for (int i = this->values.size() - 1; i >= 0; i--){
				if (this->values[i].count == 0)
					this->values.erase(this->values.begin() + i);
			}


			//Build Twees from list
			std::vector<Twee<T>> newTwees;

			int count;
			unsigned short insert = 0;
			for (unsigned int i = 0; i < list.size(); i += count){
				count = 1;
				for (unsigned int j = i; j + 1 < list.size(); j++) {
					if (list[i] == list[j + 1])
						count++;
					else break;
				}
				newTwees.push_back(Twee<T>(count, list[i]));
			}


			//Insert the TweeBlocks at the right position in the column vector
			unsigned int new_index = index != 0 ? index - 1 : 0;
			for (int i = newTwees.size() - 1; i >= 0; i--)
				this->values.insert(this->values.begin() + new_index, newTwees[i]);

			return true;
		}
		else
			return false;

	}

	template<class T>
	bool RunLengthCompressionColumn<T>::update(PositionListPtr posPtr, const boost::any& obj){
		for (unsigned int i = 0; i < posPtr->size(); i++)
			if (!this->update(posPtr->at(i), obj))
				return false;
		
		return true;
	}

	//To erase a value simply decrese the count or erase the value from 
	//the column if count == 0.
	template<class T>
	bool RunLengthCompressionColumn<T>::remove(TID tid){
		if (tid < this->elemNum) {
			int index = this->TIDtoCompressedIndex(tid);

			this->values[index].count--;
			this->elemNum--;

			if (this->values[index].count == 0)
				this->values.erase(this->values.begin() + index);

			return true;
		}

		return false;
	}

	template<class T>
	bool RunLengthCompressionColumn<T>::remove(PositionListPtr posPtr){
		for (unsigned int i = 0; i < posPtr->size(); i++)
			if (!this->remove(posPtr->at(i)))
				return false;

		return true;
	}

	template<class T>
	bool RunLengthCompressionColumn<T>::clearContent(){
		this->values.clear();
		this->elemNum = 0;
		return true;
	}

	template<class T>
	bool RunLengthCompressionColumn<T>::store(const std::string& path_){
		std::string path(path_);
		path += "/";
		path += this->name_;

		std::ofstream ofs(path.c_str(), std::ios_base::binary | std::ios_base::out);
		boost::archive::binary_oarchive oa(ofs);
		oa << (*this);

		ofs.flush();
		ofs.close();
		return true;
	}
	template<class T>
	bool RunLengthCompressionColumn<T>::load(const std::string& path_){
		std::string path(path_);
		path += "/";
		path += this->name_;

		std::ifstream ifs(path.c_str(), std::ios_base::binary | std::ios_base::in);
		boost::archive::binary_iarchive ia(ifs);
		ia >> (*this);

		ifs.close();
		return true;
	}

	template<class T>
	T& RunLengthCompressionColumn<T>::operator[](const int index){
		return values[this->TIDtoCompressedIndex(index)].value;
	}

	template<class T>
	unsigned int RunLengthCompressionColumn<T>::getSizeinBytes() const throw(){
		return this->values.capacity()*sizeof(T);
	}

}