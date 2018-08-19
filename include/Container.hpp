#ifndef CONTAINER_HPP
#define CONTAINER_HPP

#include <any>
#include <functional>
#include <map>
#include <memory>
#include <typeinfo>
#include <type_traits>

namespace cppdi
{
    class Container;

    /**
     * \brief Creates an instance of Derived class and returns a shared pointer to the Base.
     * This works only when the Derived class has a parameterless constructor. If this is not the case,
     * you should create your own template specialization.
     * \param container The dependency injection container you can use to get your class's dependencies.
     * \return Shared pointer to the base class.
     */
    template <typename Base, typename Derived>
    std::shared_ptr<Base> CreateInstance(const Container& container)
    {
        return std::make_shared<Derived>();
    }

    /**
     * \brief Represents a dependency injection container.
     * Transient means that new instances will be created every time a service is requested.
     * Singleton means that there will only be a single instance of a service.
     */
    class Container
    {
    public:
        /**
         * \brief Registers Derived class as an implementation of Base class with transient lifetime.
         */
        template <typename Base, typename Derived>
        void AddTransient()
        {
            AddService<Base, Derived>(GetTransientCreator<Base, Derived>());
        }

        /**
        * \brief Registers Derived class as an implementation of Base class with singleton lifetime.
        */
        template <typename Base, typename Derived>
        void AddSingleton()
        {
            AddService<Base, Derived>(GetSingletonCreator<Base, Derived>());
        }

        /**
         * \brief Registers the provided instance as an implementation of Type class with transient lifetime.
         * \param instance An instance of Type class to be copied on every request for Type service.
         */
        template <typename Type>
        void AddTransient(const std::shared_ptr<Type> instance)
        {
            auto creator = [instance]
            {
                return std::make_shared<Type>(instance);
            };

            AddService<Type>(creator);
        }

        /**
        * \brief Registers the provided instance as an implementation of Type class with singleton lifetime.
        * \param instance An instance of Type class to be returned on every request for Type service.
        */
        template <typename Type>
        void AddSingleton(const std::shared_ptr<Type> instance)
        {
            auto creator = [instance]
            {
                return instance;
            };

            AddService<Type>(creator);
        }

        /**
         * \brief Registers the provided function to use as a creator for Type instances.
         * \param creator Function which creates instances of Type.
         */
        template <typename Type>
        void AddTransient(const std::function<std::shared_ptr<Type>()> &creator)
        {
            AddService<Type>(creator);
        }

        /**
         * \brief Registers the provided function to use as a creator for the Type instance.
         * The provided function will be lazily evaluated on the first request for Type service.
         * \param creator Function which creates instances of Type,
         */
        template <typename Type>
        void AddSingleton(const std::function<std::shared_ptr<Type>()> &creator)
        {
            auto singletonCreator = [creator]
            {
                static auto instance = creator();
                return instance;
            };

            AddService<Type>(singletonCreator);
        }

        /**
        * \brief Registers the provided function to use as a creator for Type instances.
        * \param creator Function which creates instances of Type.
        */
        template <typename Type>
        void AddTransient(const std::function<std::shared_ptr<Type>(const Container&)> &creator)
        {
            auto transientCreator = [creator]
            {
                return creator(*this);
            };

            AddService<Type>(transientCreator);
        }

        /**
        * \brief Registers the provided function to use as a creator for the Type instance.
        * The provided function will be lazily evaluated on the first request for Type service.
        * \param creator Function which creates instances of Type,
        */
        template <typename Type>
        void AddSingleton(const std::function<std::shared_ptr<Type>(const Container&)> &creator)
        {
            auto singletonCreator = [creator]
            {
                static auto instance = creator(*this);
                return instance;
            };

            AddService<Type>(singletonCreator);
        }

        /**
         * \brief Requests an instance of Base service from the container.
         * \return Instance of a service.
         */
        template <typename Base>
        std::shared_ptr<Base> GetService() const
        {
            auto key = typeid(Base).hash_code();
            auto it = _services.find(key);
            if (it != _services.end())
            {
                return std::any_cast<std::shared_ptr<Base>>(it->second());
            }

            return nullptr;
        }

        /**
        * \brief Requests an instance of Base service from the container.
        * This method will throw if the implementation for Base is not registered.
        * \return Instance of a service.
        */
        template <typename Base>
        std::shared_ptr<Base> GetRequiredService() const
        {
            auto service = GetService<Base>();
            if (service == nullptr)
            {
                throw std::runtime_error("No implementation registered for " + std::string(typeid(Base).name()));
            }
            return service;
        }

    private:
        template <typename Base, typename Derived>
        void AddService(std::function<std::any()> serviceCreator)
        {
            static_assert(std::is_base_of<Base, Derived>());
            auto key = typeid(Base).hash_code();
            _services.emplace(key, std::move(serviceCreator));
        }

        template <typename Type>
        void AddService(std::function<std::any()> serviceCreator)
        {
            auto key = typeid(Type).hash_code();
            _services.emplace(key, std::move(serviceCreator));
        }

        template <typename Base, typename Derived>
        std::function<std::any()> GetTransientCreator()
        {
            auto creator = [&]()
            {
                return std::make_any<std::shared_ptr<Base>>(CreateInstance<Base, Derived>(*this));
            };
            return creator;
        }

        template <typename Base, typename Derived>
        std::function<std::any()> GetSingletonCreator()
        {
            auto creator = [this]()
            {
                auto key = typeid(Base).hash_code();
                auto it = _singletons.find(key);
                if (it == _singletons.end())
                {
                    std::any singletonService = GetTransientCreator<Base, Derived>()();
                    _singletons.emplace(key, singletonService);
                }
                return _singletons.at(key);
            };
            return creator;
        }

        std::map<size_t, std::function<std::any()>> _services;
        std::map<size_t, std::any> _singletons;
    };
}

#endif // CONTAINER_HPP
