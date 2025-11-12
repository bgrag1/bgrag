std::list<UnboundedHelloWorld> default_unbounded_helloworld_data_generator(
        size_t max)
{
    uint16_t index = 1;
    size_t maximum = max ? max : 10;
    std::list<UnboundedHelloWorld> returnedValue(maximum);

    std::generate(returnedValue.begin(), returnedValue.end(), [&index]
            {
                UnboundedHelloWorld hello;
                hello.index(index);
                std::stringstream ss;
                ss << "HelloWorld " << index;
                hello.message(ss.str());
                ++index;
                return hello;
            });

    return returnedValue;
}