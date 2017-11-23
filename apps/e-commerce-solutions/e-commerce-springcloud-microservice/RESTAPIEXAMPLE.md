# Order, Cart, Search curl command example
* [GetOrder](#1)
* [GetAllOrder](#2)
* [CreateOrder](#3)
* [DeleteOrder](#4)
* [GetCartByUser](#5)
* [GetCartByCartId](#6)
* [CreateCart](#7)
* [AddOrUpdateProduct](#8)
* [DeleteProduct](#9)
* [DeleteCart](#10)
* [SearchProduct](#11)

## <a name="1">GetOrder</a>

curl http://192.168.12.100:9000/v1/order/1/1

## <a name="2">GetAllOrder</a>

curl http://192.168.12.100:9000/v1/order/1

## <a name="3">CreateOrder</a>

curl -X POST  http://192.168.12.100:9000/v1/order -H "Content-Type: application/json" -d '{"addressDTO":{"naddressid":1808782},"deliveryDTO":{"naddressid":1808782,"ndeliveryprice":100,"sexpresscompany":"EMS"},"orderskudtoList":[{"discount":0,"originPrice":7632,"price":7632,"quantity":5,"skuId":1},{"discount":0,"originPrice":8743,"price":8743,"quantity":7,"skuId":2}],"scustomermark":"everything is good!","totalQuantity":0,"userId":1}'

## <a name="4">DeleteOrder</a>

curl -X  DELETE http://192.168.12.100:9000/v1/order/1/1

## <a name="5">GetCartByUser</a>

curl http://192.168.12.100:9000/v1/cart/1

## <a name="6">GetCartByCartId</a>

curl http://192.168.12.100:9000/v1/cart/1/1

## <a name="7">CreateCart</a>

curl -X POST http://192.168.12.100:9000/v1/cart -H "Content-Type: application/json" -d '{"currency":"RMB","discount":0,"quantity":0,"skudtoList":[{"discount":0,"displayPrice":8743,"quantity":10,"skuId":2,"spuId":1},{"discount":0,"displayPrice":7632,"quantity":10,"skuId":1,"spuId":1}],"userId":1}'

## <a name="8">AddOrUpdateProduct</a>

curl -X  POST http://192.168.12.100:9000/v1/cart/1/1/skus/3 -H "Content-Type: application/json" -d '{"currency":"RMB","discount":0,"quantity":0,"skudtoList":[{"discount":0,"displayPrice":2564,"quantity":20}]}'

curl -X  POST http://192.168.12.100:9000/v1/cart/1/1/skus/3 -H "Content-Type: application/json" -d '{"currency":"RMB","discount":0,"quantity":0,"skudtoList":[{"discount":0,"displayPrice":2564,"quantity":100}]}'

## <a name="9">DeleteProduct</a>

curl -X  DELETE http://192.168.12.100:9000/v1/cart/1/1/skus/3

## <a name="10">DeleteCart</a>

curl -X  DELETE http://192.168.12.100:9000/v1/cart/1/1

## <a name="11">SearchProduct</a>

curl http://192.168.12.100:9000/v1/search?query=*:*\&page_size=10\&page_num=1\&sort=

