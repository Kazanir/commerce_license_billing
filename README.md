Commerce License Billing
========================

Commerce License Billing provides advanced (prepaid, postpaid, prorated, plan-based, metered)
recurring billing for licenses.

Dependencies: Bundleswitcher, Commerce License, Advanced queue.

Getting started
---------------
1. Go to admin/config/licenses/billing-cycle-types and add a billing cycle type.
2. Create a product, select a license type, then below select your billing cycle type
and billing type (prepaid or postpaid).
3. Checkout the product. If you selected postpaid as the billing type, your product
will be free.
4. A billing cycle has now been opened (with the current start date, and the end date
depending on your billing cycle type settings), along with a matching recurring order.
5. When the billing cycle expires, the recurring order will be closed and charged for
using Commerce Card on File, and a new billing cycle & order will be opened.

Relationship to Commerce License
--------------------------------
Any license type can be used for recurring billing without changes.
A license is considered billable if its product has a billing cycle type
selected.

Billing cycle types
-------------------
Billing cycle types are exportable entities managed on admin/config/licenses/billing-cycle-types.
Each billing cycle type has a bundle, the billing cycle engine, powered by
[entity bundle plugin](https://drupal.org/project/entity_bundle_plugin).
This allows each billing cycle type to have methods for creating and naming new billing cycles.
See `CommerceLicenseBillingCycleEngineInterface` and `CommerceLicenseBillingCycleTypeBase`
for more information.

The module provides a "periodic" billing cycle engine that generates periodic
billing cycles (daily/weekly/monthly, synchronous or asynchronous).

Fields and bundles
------------------
The module creates the following fields on any product type that's license enabled:

- `cl_billing_cycle_type` - Reference to the billing cycle type.
- `cl_billing_type` - The billing type (prepaid / postpaid).

It provides the recurring line item type with the following additional fields:

- `cl_billing_start` - a datetime field, used for prorating.
- `cl_billing_end` - a datetime field, used for prorating.
- `cl_billing_license` - Reference to the license for which the line item was generated.

It provides the recurring order type with the following additional fields:

- `cl_billing_cycle` - Reference to the billing cycle.
