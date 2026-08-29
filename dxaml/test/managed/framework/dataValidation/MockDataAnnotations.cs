// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.ComponentModel;
using System.Collections;
using System.Reflection;

// Ideally we wouldn't need to do this. However DataAnnotations wasn't introduced into the 
// CoreCLR until v5 and we can only build against 4.x so we can't use it in Razzle.
namespace MockDataAnnotations
{
    public struct ValidationContext
    {
        public ValidationContext(object context, string memberName)
        {
            Object = context;
            MemberName = memberName;
        }
        public object Object {get; set;}
        public string MemberName {get; set;}
    }

    public class ValidationResult
    {
        public ValidationResult(string errorMessage, string memberName)
        {
            ErrorMessage = errorMessage;
            MemberNames = new List<string>(){memberName};
        }

        public string ErrorMessage
        {
            get;
            set;
        }

        public List<string> MemberNames {get; private set;}
    }

    public static class Validator
    {
        public static bool TryValidateObject(object obj, out List<ValidationResult> results)
        {
            results = new List<ValidationResult>();
            var typeInfo = obj.GetType().GetTypeInfo();
            foreach (var attr in typeInfo.CustomAttributes)
            {
                if (typeInfo.GetCustomAttribute(attr.AttributeType) is ValidationAttribute validation)
                {
                    if (!validation.IsValid(obj))
                    {
                        results.Add(new ValidationResult(validation.ErrorMessage, ""));
                        return false;
                    }
                }
            }

            return true;
        }

        public static bool TryValidateProperty(object value, ValidationContext context, out List<ValidationResult> results)
        {
            results = new List<ValidationResult>();
            var typeInfo = context.Object.GetType().GetTypeInfo();
            var prop = typeInfo.DeclaredProperties.Single(p => p.Name == context.MemberName);
            foreach (var attr in prop.CustomAttributes)
            {
                if (prop.GetCustomAttribute(attr.AttributeType) is ValidationAttribute validation)
                {
                    if (!validation.IsValid(value))
                    {
                        results.Add(new ValidationResult(validation.ErrorMessage, context.MemberName));
                        return false;
                    }
                }
            }

            return true;
        }
    }

    public abstract class ValidationAttribute : Attribute
    {
        public string ErrorMessage{get; set;}
        public abstract bool IsValid(object obj);
    }

    public class PhoneNumberAttribute : ValidationAttribute
    {
        public PhoneNumberAttribute()
        {
            ErrorMessage = "Invalid phone number";
        }

        public PhoneNumberAttribute(string errorMessage)
        {
            ErrorMessage = errorMessage;
        }

        public override bool IsValid(object obj)
        {
            if (obj is string valueString )
            {
                // Not trying to be too correct. Phone number must be 10 digits
                // and only numbers.
                return valueString.Length == 10 && valueString.All(c => Char.IsNumber(c));
            }

            return false;
        }
    }

    public class MinAttribute : ValidationAttribute
    {
        public int Min
        {
            get;
            set;
        }
    
        public MinAttribute(int min) : this(min, "Value too low")
        {
        }

        public MinAttribute(int min, string errorMessage)
        {
            ErrorMessage = errorMessage;
            Min = min;
        }


        public override bool IsValid(object obj)
        {
            if (obj is string valueString )
            {
                if (int.TryParse(valueString, out int value))
                {
                    return value >= Min;
                }
            }

            return false;
        }
    }

    public class RequiredAttribute : ValidationAttribute
    {
        public RequiredAttribute()
        {
            ErrorMessage = "Value is required";
        }

        public override bool IsValid(object obj)
        {
            // Object can't be null, or if it's a string it shouldn't be empty
            if (obj == null) return false;

            if (obj is string str)
            {
                return !String.IsNullOrEmpty(str);
            }

            return true;
        }
    }
}