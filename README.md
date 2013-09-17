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

By default, licenses are revisionable, but changes to a license don't create
new revisions. Commerce License Billing changes that logic for billable licenses,
ensuring that a new revision is created for status or product_id changes
(this is essential for later pricing and prorating).

Plan-based billing
------------------
Each license has one plan at a given point of time, which is the referenced product.

Licenses are revisionable, and Commerce License Billing modifies the default
behavior so that new revisions are always created for product_id (plan) and
status changes. Each revision has a `revision_created` and `revision_ended` timestamp.
The `revision_ended` timestamp is 0 for the current revision.
The timestamps are used to prorate the price of each plan that was active
during the billing cycle.

Only revisions with status `COMMERCE_LICENSE_ACTIVE` are priced, so if a license
was inactive for a week, that period won't be priced since that revision will
be ignored.

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
