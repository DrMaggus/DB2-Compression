#pragma once

#include <core/compressed_column.hpp>
#include <vector>

namespace CoGaDB{

	template<typename T>
	struct Twee {
		int count;
		T value;
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
		//assumes tid list is sorted ascending
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

	private:
		int TIDtoCompressedIndex(TID tid);
		std::vector<Twee<T>> values;
		int elemNum;
	};

	template<class T>
	RunLengthCompressionColumn<T>::RunLengthCompressionColumn(const std::string &name, AttributeType db_type) : CompressedColumn<T>(name, db_type){
		elemNum = 0;
	}

	template<class T>
	RunLengthCompressionColumn<T>::~RunLengthCompressionColumn(){

	}

	template<class T>
	int RunLengthCompressionColumn<T>::TIDtoCompressedIndex(TID tid) {
		int tid_ = 0;
		for (int i = 0; i < this->values.size(); i++) {
			tid_ += this->values[i].count;
			if (tid_ > tid)
				return i;
		}
	}

	template<class T>
	bool RunLengthCompressionColumn<T>::insert(const T &new_value){
		if (this->values.empty() || this->values[this->values.size() - 1].value != new_value)
		{
			Twee<T> tuple;
			tuple.count = 1;
			tuple.value = new_value;
			this->values.push_back(tuple);
			this->elemNum++;
			return true;
		} else {
			this->values[this->values.size() - 1].count++;
			this->elemNum++;
			return true;
		}

		return false;
	}

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
		for (unsigned int i = 0; i<values.size(); i++){
			std::cout << "| " << values[i].count << " | "<< values[i].value << " |" << std::endl;
		}

	}
	template<class T>
	size_t RunLengthCompressionColumn<T>::size() const throw(){
		return this->values.size();
	}
	template<class T>
	const ColumnPtr RunLengthCompressionColumn<T>::copy() const{
		return ColumnPtr(new RunLengthCompressionColumn(*this)); //because no dynamic memory allocations or pointer variables
	}

	template<class T>
	bool RunLengthCompressionColumn<T>::update(TID tid, const boost::any& obj){
		int tid_ = 0, index;
		for (index = 0; i < this->values.size(); i++) {
			for (int j = 0; j < this->values[i].count; j++) {
				printf("%d   - %d   %s \n", tid_++, tid, tid_==tid);
				if (tid_ == tid)

		}
		/*
		for ()
		int index = this->TIDtoCompressedIndex(tid);
		if (index < this->elemNum || typeid(T) != obj.type() {
			if (this->values[index].value == boost::any_cast<T>(obj)) return true; //nothing to do
			if (this->value[index].count == 1)
				this->values[index].value == boost::any_cast<T>(obj); //just insert value
			else if ()

		}*/
		return false;
	}

	template<class T>
	bool RunLengthCompressionColumn<T>::update(PositionListPtr, const boost::any& obj){
		return false;
	}

	template<class T>
	bool RunLengthCompressionColumn<T>::remove(TID tid){
		return false;
	}

	template<class T>
	bool RunLengthCompressionColumn<T>::remove(PositionListPtr ptr){
		return false;
	}

	template<class T>
	bool RunLengthCompressionColumn<T>::clearContent(){
		return false;
	}

	template<class T>
	bool RunLengthCompressionColumn<T>::store(const std::string& path){
		return false;
	}
	template<class T>
	bool RunLengthCompressionColumn<T>::load(const std::string& path){
		return false;
	}

	template<class T>
	T& RunLengthCompressionColumn<T>::operator[](const int index){
		static T t;
		return t;
	}

	template<class T>
	unsigned int RunLengthCompressionColumn<T>::getSizeinBytes() const throw(){
		return this->values.capacity()*sizeof(T);
	}

}