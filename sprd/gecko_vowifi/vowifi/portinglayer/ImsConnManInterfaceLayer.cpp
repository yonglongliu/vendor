#include "ImsConnManInterfaceLayer.h"
#include "imsconnman/ImsConnManMonitor.h"
#include "mozilla/ModuleUtils.h"
#include "mozilla/ClearOnShutdown.h"


BEGIN_VOWIFI_NAMESPACE

const string ImsConnManInterfaceLayer::TAG = "ImsConnManInterfaceLayer";

NS_IMPL_ISUPPORTS(ImsConnManInterfaceLayer, nsIImsConnManInterfaceLayer)

StaticRefPtr<ImsConnManInterfaceLayer> ImsConnManInterfaceLayer::sSingleton;

already_AddRefed<ImsConnManInterfaceLayer> ImsConnManInterfaceLayer::GetInstance() {
    if (!sSingleton) {
        LOGD(_PTAG(TAG), "GetInstance: new sSingleton");
        sSingleton = new ImsConnManInterfaceLayer();
        ClearOnShutdown(&sSingleton);
    }

    RefPtr<ImsConnManInterfaceLayer> service = sSingleton.get();
    return service.forget();
}

ImsConnManInterfaceLayer::ImsConnManInterfaceLayer() {
}

ImsConnManInterfaceLayer::~ImsConnManInterfaceLayer() {
}

NS_IMETHODIMP ImsConnManInterfaceLayer::Init() {
    LOGD(_PTAG(TAG), "Init: invoke ImsConnManMonitor::getInstance().onInit()");
    ImsConnManMonitor::getInstance().onInit();

    return NS_OK;
}


NS_GENERIC_FACTORY_SINGLETON_CONSTRUCTOR(ImsConnManInterfaceLayer,
        ImsConnManInterfaceLayer::GetInstance)

NS_DEFINE_NAMED_CID(NS_IMSCONNMANINTERFACELAYER_CID);

static const mozilla::Module::CIDEntry kImsConnManInterfaceLayerCIDs[] = {
        { &kNS_IMSCONNMANINTERFACELAYER_CID, false, nullptr, ImsConnManInterfaceLayerConstructor },
        { nullptr }
};

static const mozilla::Module::ContractIDEntry kImsConnManInterfaceLayerContracts[] = {
        { IMSCONNMANINTERFACELAYER_CONTRACTID, &kNS_IMSCONNMANINTERFACELAYER_CID },
        { nullptr }
};

static const mozilla::Module kImsConnManInterfaceLayerModule = {
        mozilla::Module::kVersion,
        kImsConnManInterfaceLayerCIDs,
        kImsConnManInterfaceLayerContracts,
        nullptr };

NSMODULE_DEFN (ImsConnManInterfaceLayerModule) = &kImsConnManInterfaceLayerModule;

END_VOWIFI_NAMESPACE
