namespace mathvm {
    struct StackVar {
        union {
            int64_t intVal;
            double doubleVal;
            uint16_t stringId;
        };

        StackVar() {
            intVal = 0;
        }

        template<class T>
        void setVal(T val);

        template<class T>
        T val();
    };
}