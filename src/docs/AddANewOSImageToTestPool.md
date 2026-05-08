# How to Add a New OS Image to the WinDevPool-Test Pool

## Table of Contents

- [Scenario](#scenario)
- [Step-by-step Instructions (Marketplace Image)](#step-by-step-instructions-marketplace-image)
  - [Start Creating a New Test Pool Image](#start-creating-a-new-test-pool-image)
  - [Specify Basic Properties](#specify-basic-properties)
  - [Pick a Base Image from the Market Place](#pick-a-base-image-from-the-market-place)
  - [Customize Your Image](#customize-your-image)
  - [Review+Create Your Image](#reviewcreate-your-image)
  - [Add the New Win11-24h2 Image to Your WinDevPool-Test Pool](#add-the-new-win11-24h2-image-to-your-windevpool-test-pool)
  - [Update Your Test matrix](#update-your-test-matrix)
  - [Notes](#notes)
- [Using an Internal Branch Image from DevDiv Image Gallery](#using-an-internal-branch-image-from-devdiv-image-gallery)
  - [Get Access to DevDiv Image Gallery](#get-access-to-devdiv-image-gallery)
  - [Check If Your Required Image Is Available](#check-if-your-required-image-is-available)
  - [Create the Test Pool Image Using the Gallery Image](#create-the-test-pool-image-using-the-gallery-image)

## Scenario

This doc outlines the basic steps for creating a new OS image, say, Win11-24h2 for use in the test stage.
Assume that we want Win11-24h2 to be essentially a clone of the an existing OS image, say, Win11-23h2. This is the most common situation.
Ultimately, we want to add the new Win11-24h2 OS image to the WinDevPool-Test pool, which Win11-23h2 is already in.

There are two approaches depending on the base image source:
1. **Marketplace image** - Use a publicly available Windows image from Azure Marketplace (e.g., Win 11 Pro 24h2). This is the most common case and is covered in the first section below.
2. **Internal branch image** - Use an image from the DevDiv Image Gallery (e.g., a `ge_current` build). This is useful when you need a VM running a specific internal Windows branch. See [Using an Internal Branch Image from DevDiv Image Gallery](#using-an-internal-branch-image-from-devdiv-image-gallery).

## Step-by-step Instructions (Marketplace Image)

### Start Creating a New Test Pool Image
•	Go to the Azure Portal CloudTest blade, hit the "Create image" button.

### Specify Basic Properties
- Fill out these 4 properties in the picture: Subscription, Resource Group, Friendly Name, and Region properties. If in doubt, consult an existing image like Win11-23h2.
- Then click the “browse all public and private images” button; well, unless you already found the image you want in the drop down menu for the Image property.

### Pick a Base Image from the Market Place

- Go under Windows 11.
- Hit its Select button to open the menu; aha, the Win 11 Pro 24h2 down there looks nice, select it.

### Customize Your Image

- You should now see “Windows 11 Pro, version 24h2” in the image field. 
- Click the “Define a custom image” button. 
- Go to your existing Win11-23h2 image, copy the contents in the Artifacts property and paste it into Win11-24h2's Artifacts.
- There are many other properties you can set now or later. More often than not, you just need to copy properties from an existing OS image.
- It'd be great if you can put a couple of emails as contacts for your new image.
- But the above is the minimum you need to do to create a new custom test image.

### Review+Create Your Image

- Click through the Review+create button to start creating the new image. It can take a couple of hours.

### Add the New Win11-24h2 Image to Your WinDevPool-Test Pool

- Once your image has been created and deployed successfully, you can go to your WinDevPool-Test screen. 
- Now Win11-24h2 should be available to be selected and added to WinDevPool-Test.
- Congratulations! That's all you need to do in Azure. 

### Update Your Test matrix

Now you typically want to update your yml file to add the new Win11-24h2 image to your test matrix.

### Notes

- arm64 and x64 are of different VM types, so they cannot coexist in the same pool. Typically, there are 2 serparate pools, one for each architecture. Hence, don't try to add an arm64 image to a x64 pool, and vice versa.

---

## Using an Internal Branch Image from DevDiv Image Gallery

If you need a VM running a specific internal Windows branch (e.g., `ge_current`), you can use images from the DevDiv Image Gallery instead of picking one from the Azure Marketplace.

### Get Access to DevDiv Image Gallery

To see the list of available images in the gallery, request access to the ImageFactory API by joining the appropriate access group.

### Check If Your Required Image Is Available

Browse the DevDiv Image Gallery to see if the image you need already exists (e.g., `ge_current` x64 or arm64).

If the image you need is **not** available, you can request it to be added by following the internal Image Factory documentation.

### Create the Test Pool Image Using the Gallery Image

1. Go to the Azure Portal CloudTest blade and hit "Create image".
2. Fill out Subscription, Resource Group, Friendly Name, and Region. For Region, choose the same region where the image is available in the gallery.
3. For Image Type, select **Define a custom image (managed)**.
4. Leave **Base Image Type** and **Image** fields empty.
5. In the **Base Image Resource ID** field, enter the resource ID of the gallery image. For example:
   ```
   /subscriptions/<subscription-id>/resourceGroups/DevDivImageGallery/providers/Microsoft.Compute/galleries/DevDivImageGallery/images/ge_current-vhdx_client_enterprise_en-us_vl-arm64/versions/latest
   ```
   Modify the image name portion of the path to match the image available in the gallery for your scenario.
6. Add artifacts similar to other existing images, or as required for your scenario.
7. If you are creating an **ARM64** image, go to **Next: Advanced** and change the **Sku** to **Standard D8pls v5**.
8. Click **Review + Create**.

Once the image is built, the steps for adding it to the WinDevPool-Test pool and updating the test matrix are the same as described above for a [Marketplace image](#add-the-new-win11-24h2-image-to-your-windevpool-test-pool).
