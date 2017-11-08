## <a name="1">CartReq</a>
{
    "currency": ,
    "quantity": ,
    "skudtoList": [
        {
            "discount": ,
            "displayPrice": ,
            "quantity": ,
            "skuId": 
        },
        {
            "discount": ,
            "displayPrice": ,
            "quantity": ,
            "skuId": 
        }
    ],
    "userId": 
}

## <a name="2">CartRes</a>

{
    "nshoppingcartid": ,
    "userId": ,
    "discount": ,
    "price": ,
    "quantity": ,
    "currency": ,
    "skudtoList": [
        {
            "skuId": ,
            "spuId": ,
            "color": ,
            "size": ,
            "price": ,
            "displayPrice": ,
            "currency": ,
            "discount": ,
            "dcreatetime": ,
            "dupdatetime": ,
            "quantity": 
        },
        {
            "skuId": ,
            "spuId": ,
            "color": ,
            "size": ,
            "price": ,
            "displayPrice": ,
            "currency": ,
            "discount": ,
            "dcreatetime": ,
            "dupdatetime": ,
            "quantity": 
        }
    ],
    "supdatetime": ,
    "ncreatetime": 
}

## <a name="3">OrderReq</a>
{
    "addressDTO": {
        "naddressid": 
    },
    "deliveryDTO": {
        "naddressid": ,
        "ndeliveryprice": ,
        "sexpresscompany": 
    },
    "orderskudtoList": [
        {
            "discount": ,
            "originPrice": ,
            "price": ,
            "quantity": ,
            "skuId": 
        },
        {
            "discount": ,
            "originPrice": ,
            "price": ,
            "quantity": ,
            "skuId": 
        }
    ],
    "scustomermark": ,
    "totalQuantity": ,
    "userId": 
}

## <a name="4">OrderRes</a>
{
    "orderId": ,
    "userId": ,
    "parentOrderid": ,
    "paymentMethod": ,
    "discount": ,
    "totalQuantity": ,
    "totalPrice": ,
    "cstatus": ,
    "screatetime": ,
    "supdatetime": ,
    "scompletedtime": ,
    "scustomermark": ,
    "sordercode": ,
    "sshopcode": ,
    "sordertype": ,
    "dpaymenttime": ,
    "sordersource": ,
    "addressDTO": {
        "naddressid": ,
        "nuserid": ,
        "sfirstname": ,
        "slastname": ,
        "saddress": ,
        "scity": ,
        "sprovince": ,
        "scountry": ,
        "semail": ,
        "sphoneno": ,
        "sdistrict": ,
        "dcreatetime": ,
        "dupdatetime": ,
        "szipcode": 
    },
    "deliveryDTO": {
        "ndeliveryid": ,
        "sexpresscompany": ,
        "ndeliveryprice": ,
        "cstatus": ,
        "dcreatetime": ,
        "dupdatetime": ,
        "douttime": ,
        "naddressid": ,
        "sconsignee": ,
        "sdeliverycomment": ,
        "sdeliverycode": 
    },
    "orderskudtoList": [
        {
            "orderId": ,
            "skuId": ,
            "quantity": ,
            "originPrice": ,
            "discount": ,
            "currency": ,
            "price": 
        },
        {
            "orderId": ,
            "skuId": ,
            "quantity": ,
            "originPrice": ,
            "discount": ,
            "currency": ,
            "price": 
        }
    ]
}

## <a name="5">Status</a>

{
    "userId": ,
    "status": 
}

## <a name="6">Sku</a>

{
    "nskuid": ,
    "nspuid": ,
    "ncolor": ,
    "ssize": ,
    "nprice": ,
    "ndisplayprice": ,
    "ninventory": ,
    "ndiscount": ,
    "scurrency": ,
    "dcreatetime": ,
    "dupdatetime": 
}

