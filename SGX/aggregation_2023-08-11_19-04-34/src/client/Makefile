######## App Settings ########

App_Cpp_Files := isv_app/isv_app.cpp
App_Include_Paths := -Iservice_provider -I../common -Iinclude 

App_C_Flags := -fPIC -Wno-attributes $(App_Include_Paths)

App_Cpp_Flags := $(App_C_Flags)
App_Link_Flags := -L. -lpthread -lservice_provider -Wl,-rpath=$(CURDIR)/sample_libcrypto -Wl,-rpath=$(CURDIR)

App_Cpp_Objects := $(App_Cpp_Files:.cpp=.o)

App_Name := aggClient

######## Service Provider Settings ########

ServiceProvider_Cpp_Files := service_provider/ecp.cpp service_provider/network_ra.cpp service_provider/service_provider.cpp service_provider/ias_ra.cpp service_provider/ra_client.cpp service_provider/remote_attestation.cpp service_provider/learning_data.cpp service_provider/schedule_config.cpp service_provider/app_config.cpp service_provider/aggregation.cpp service_provider/agg_timer.cpp
ServiceProvider_C_Flags := -fPIC -Wno-attributes -Isample_libcrypto -I../common
ServiceProvider_Cpp_Flags := $(ServiceProvider_C_Flags) -std=c++11
ServiceProvider_Link_Flags :=  -shared -lsample_libcrypto -Lsample_libcrypto

ServiceProvider_Cpp_Objects := $(ServiceProvider_Cpp_Files:.cpp=.o)




.PHONY: all run target
all: .config
	@$(MAKE) target

target: libservice_provider.so $(App_Name) 

.config:
	@rm -f .config $(App_Name) $(App_Name) $(App_Cpp_Objects) libservice_provider.* $(ServiceProvider_Cpp_Objects)
	@touch .config

######## App Objects ########
isv_app/%.o: isv_app/%.cpp 
	@$(CXX) $(App_Cpp_Flags) -c $< -o $@
	@echo "CXX  <=  $<"

$(App_Name): $(App_Cpp_Objects)
	@$(CXX) $^ -o $@ $(App_Link_Flags)
	@echo "LINK =>  $@"

######## Service Provider Objects ########

service_provider/%.o: service_provider/%.cpp
	@$(CXX)  $(ServiceProvider_Cpp_Flags) -c $< -o $@
	@echo "CXX  <=  $<"

libservice_provider.so: $(ServiceProvider_Cpp_Objects)
	@$(CXX) $^ -o $@ $(ServiceProvider_Link_Flags)
	@echo "LINK =>  $@"


.PHONY: clean

clean:
	@rm -f .config_* $(App_Name) $(App_Cpp_Objects) libservice_provider.* $(ServiceProvider_Cpp_Objects)
